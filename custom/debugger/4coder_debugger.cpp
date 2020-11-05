#undef function
#include "lldb/API/LLDB.h"
#define function static
#include <stdio.h>
#include <string.h>

////////////////////////////
//TODOS:
//-when running ctrl+f10 it actually recompiles the code if any changes were made, so write a feature that allows to compile if any changes were made
//-add watchpoints and "logpoints"
//-add watch window (or watches)
//-make sure the user doesn't accidentally opens a random (ie *scratch*) buffer because then the debugger wont work properly and theres no way up as of now to verify it
//-get out of loop (consider setting a one off breakpoint after the loop and hit continue)
//-handle segfaults ? fixed?
//-what happens if you press the debugger begin twice?!
//add if(buffer found ) similar to the function create_buffer_or_switch_by_name_and_clear
//- remove lit_expr_u8?!
//- can lldb tell the difference between garbage value and a NULL?
//-inline variable values in source code, similar to clion
//-paste inside query bar? automatically search for executable? use special buffer for pasting and seraching? 
//-handle segfaulting at the end
///////////////////////////

using namespace lldb;

//#define testing_debugger 1

struct debugger_breakpoint {
    SBBreakpoint breakpoint;
    char breakpoint_location_file_name[50];
    i32 line_num;
};

#define MAX_BREAKPOINTS 100 //TODO:(wormboy) chagne this later on

struct debugger_info {
    //TODO(wormboy): consider chaging it to c code
    b32 is_running = false;
    
    SBDebugger debugger;
    SBTarget target;
    SBProcess process;
    
    debugger_breakpoint breakpoints[MAX_BREAKPOINTS] = {};
    i32 breakpoints_count = 0;
    
    i32 current_line_num;
    const char *current_file;
    
    //Buffer_Id debugger_buffer_id;
    
    //const char *debugger_target_name
    
    char evaluation_strings[20][50] = {};
    int evaluation_strings_count = 0;
    
} g_debugger_info;

const char *special_buffer_name = "*special*";

//HELPER//

void debugger_print_error_func(const char *func, int line, const char *error) {
    fprintf(stderr, "%s %d: %s\n", func,  line, error);
}

#define debugger_print_error(error)  debugger_print_error_func(__FUNCTION__, __LINE__, error)

//EXECUTION COMMANDS//


void thread_run_until_line(int line_num) {
    SBThread thread = g_debugger_info.process.GetThreadAtIndex(0);
    SBFrame frame = thread.GetFrameAtIndex(0);
    SBFileSpec file_spec = g_debugger_info.target.GetExecutable();
    thread.StepOverUntil(frame, file_spec, line_num);
}

//EXAMINING THREAD STATE//
void debugger_print_backtrace(Buffer_Insertion *out) {
    SBThread thread = g_debugger_info.process.GetThreadAtIndex(0);
    insertf(out, "-------------[backtrace]-------------\n");
    for (uint32_t i = 0; i < thread.GetNumFrames(); ++i) {
        SBFrame frame = thread.GetFrameAtIndex(i);
        //TODO: clean this up
        insertf(out, "frame #%d: %s\n", i, frame.GetDisplayFunctionName());
    }
}

void debugger_print_registers(Buffer_Insertion *out) {
    SBThread thread = g_debugger_info.process.GetThreadAtIndex(0);
    SBFrame frame = thread.GetFrameAtIndex(0);
    SBValueList registers_value_list = frame.GetRegisters();
    insertf(out, "-------------[registers]-------------\n");
    for (uint32_t i = 0; i < registers_value_list.GetSize(); ++i) {
        SBValue value = registers_value_list.GetValueAtIndex(i);
        //printf("%s\n------------------------\n", value.GetName());
        for (uint32_t i = 0; i < value.GetNumChildren(); ++i) {
            SBValue register_value = value.GetChildAtIndex(i);
            insertf(out, "%s %s\n", register_value.GetName(), register_value.GetValue());
        }
    }
}

//BREAKPOINT COMMANDS//

i64 get_cursor_line_number(Application_Links *app, View_ID view_id) {
    i64 pos = view_get_cursor_pos(app, view_id);
    Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(pos));
    return cursor.line;
}



void debugger_add_breakpoint_on_buffer(Application_Links *app, i32 line_number, Buffer_ID buffer_id) {
    Scratch_Block scratch(app);
    String_Const_u8 buffer_file_name = push_buffer_file_name(app, scratch, buffer_id);
    String_Const_u8 file_name;
    for (int i = buffer_file_name.size; ; --i) {
        if (buffer_file_name.str[i] == '/') {
            file_name = SCu8(buffer_file_name.str + i + 1);
            break;
        }
    }
    
    SBBreakpoint breakpoint = g_debugger_info.target.BreakpointCreateByLocation((const char *)file_name.str, line_number);
    
    if (!breakpoint.IsValid()) {
        //NOTE(wormboy): this wont work all the time, like in the case where it cant find the file and it will report everything is normal
        debugger_print_error( "breakpoint is not valid");
        return;
    }
    
#if 0
    g_debugger_info.breakpoints[g_debugger_info.breakpoints_count++] = {
        .breakpoint = breakpoint,
        .breakpoint_location_file_name = (const char *)buffer_file_name.str,
        .line_num = line_number
    };
#else
    g_debugger_info.breakpoints[g_debugger_info.breakpoints_count].breakpoint = breakpoint;
    //g_debugger_info.breakpoints[g_debugger_info.breakpoints_count].breakpoint_location_file_name = (const char *)buffer_file_name.str;
    strcpy(g_debugger_info.breakpoints[g_debugger_info.breakpoints_count].breakpoint_location_file_name, (const char *)buffer_file_name.str);
    g_debugger_info.breakpoints[g_debugger_info.breakpoints_count].line_num = line_number;
    g_debugger_info.breakpoints_count++;
#endif
}

void debugger_add_breakpoint(Application_Links *app, i32 line_number) {
    //NOTE(wormboy): lldb will get upset if you give him /some/path/file.c format so you have to parse it- will require further investigation
    View_ID view_id = get_active_view(app, Access_Always);
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_Always);
    debugger_add_breakpoint_on_buffer(app, line_number, buffer_id);
}

i32 debugger_get_current_line(Application_Links *app) {
    View_ID view_id = get_active_view(app, Access_Always);
    i32 line_num = get_cursor_line_number(app, view_id);
    
    return line_num;
}

void draw_variables(Buffer_Insertion *out ,SBValue value, int depth) {
    if (value.MightHaveChildren()) {
        insertf(out, "%*s> %s %s \n", depth, "", value.GetTypeName(), value.GetName());
        depth++;
        for (int i = 0; i < value.GetNumChildren(); ++i) {
            //fprintf(stderr, "value: %s and %d\n", value.GetName(), i);
            SBValue child_value = value.GetChildAtIndex(i);
            draw_variables(out, child_value, depth);
        }
    }
    else {
        insertf(out, "%*s> %s %s = %s\n", depth, "", value.GetTypeName(), value.GetName(), value.GetValue());
    }
}

void debugger_print_variables(Buffer_Insertion *out) {
    SBThread thread = g_debugger_info.process.GetThreadAtIndex(0);
    SBFrame frame = thread.GetFrameAtIndex(0);
    SBValueList variables_value_list = frame.GetVariables(true, true, true ,true);
    insertf(out, "-------------[variables]-------------\n");
    for (uint32_t i = 0; i < variables_value_list.GetSize(); ++i) {
        SBValue value = variables_value_list.GetValueAtIndex(i);
#if 0
        insertf(out, "value: %s type_name: %s\n display_type_name:%s\n summary:%s\n description:%s\n----\n", value.GetName(), value.GetTypeName(), value.GetDisplayTypeName(), value.GetSummary(), value.GetObjectDescription());
#else
        draw_variables(out ,value, 0);
        
#endif
        
    }
}


void F4_RenderBraceLines(Application_Links *app, Buffer_ID buffer, View_ID view, Text_Layout_ID text_layout_id, Face_ID face_id, i64 pos)
{
    ProfileScope(app, "[Fleury] Brace Annotation");
    
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    
    if(token_array.tokens != 0)
    {
        Token_Iterator_Array it = token_iterator_pos(0, &token_array, pos);
        Token *token = token_it_read(&it);
        
        if(token != 0 && token->kind == TokenBaseKind_ScopeOpen)
        {
            pos = token->pos + token->size;
        }
        else if(token_it_dec_all(&it))
        {
            token = token_it_read(&it);
            if (token->kind == TokenBaseKind_ScopeClose &&
                pos == token->pos + token->size)
            {
                pos = token->pos;
            }
        }
    }
    
    Scratch_Block scratch(app);
    Range_i64_Array ranges = get_enclosure_ranges(app, scratch, buffer, pos, RangeHighlightKind_CharacterHighlight);
    
    for (i32 i = ranges.count - 1; i >= 0; i -= 1)
    {
        Range_i64 range = ranges.ranges[i];
        
        if(range.start >= visible_range.start)
        {
            continue;
        }
        
        Rect_f32 close_scope_rect = text_layout_character_on_screen(app, text_layout_id, range.end);
        Vec2_f32 close_scope_pos = { close_scope_rect.x0 + 12, close_scope_rect.y0 };
        
        // NOTE(rjf): Find token set before this scope begins.
        Token *start_token = 0;
        i64 token_count = 0;
        {
            Token_Iterator_Array it = token_iterator_pos(0, &token_array, range.start-1);
            int paren_nest = 0;
            
            for(;;)
            {
                Token *token = token_it_read(&it);
                
                if(token)
                {
                    token_count += 1;
                    
                    if(token->kind == TokenBaseKind_ParentheticalClose)
                    {
                        ++paren_nest;
                    }
                    else if(token->kind == TokenBaseKind_ParentheticalOpen)
                    {
                        --paren_nest;
                    }
                    else if(paren_nest == 0 &&
                            (token->kind == TokenBaseKind_ScopeClose ||
                             (token->kind == TokenBaseKind_StatementClose && token->sub_kind != TokenCppKind_Colon)))
                    {
                        break;
                    }
                    else if((token->kind == TokenBaseKind_Identifier || token->kind == TokenBaseKind_Keyword ||
                             token->kind == TokenBaseKind_Comment) &&
                            !paren_nest)
                    {
                        start_token = token;
                        break;
                    }
                    
                }
                else
                {
                    break;
                }
                
                if(!token_it_dec_non_whitespace(&it))
                {
                    break;
                }
            }
            
        }
        
        // NOTE(rjf): Draw.
        if(start_token)
        {
            draw_string(app, face_id, string_u8_litexpr("<-"), close_scope_pos, finalize_color(defcolor_comment, 0));
            close_scope_pos.x += 28;
            String_Const_u8 start_line = push_buffer_line(app, scratch, buffer,
                                                          get_line_number_from_pos(app, buffer, start_token->pos));
            
            u64 first_non_whitespace_offset = 0;
            for(u64 c = 0; c < start_line.size; ++c)
            {
                if(start_line.str[c] <= 32)
                {
                    ++first_non_whitespace_offset;
                }
                else
                {
                    break;
                }
            }
            start_line.str += first_non_whitespace_offset;
            start_line.size -= first_non_whitespace_offset;
            
            //NOTE(rjf): Special case to handle CRLF-newline files.
            if(start_line.str[start_line.size - 1] == 13)
            {
                start_line.size -= 1;
            }
            
            u32 color = finalize_color(defcolor_comment, 0);
            color &= 0x00ffffff;
            color |= 0x80000000;
            draw_string(app, face_id, start_line, close_scope_pos, color);
            
        }
        
    }
    
}

//4CODER STUFF//
char *debug_buffer_name = "*debug*";

void debugger_draw_current_line_annotation(Application_Links *app, Buffer_ID buffer_id, Text_Layout_ID text_layout_id, Face_ID face_id) {
#if 0
    Scratch_Block scratch(app);
    String_Const_u8 string_to_draw = push_stringf(scratch, "%s", "<=current line ");
    int line_num = g_debugger_info.process.GetSelectedThread().GetFrameAtIndex(0).GetLineEntry().GetLine();
    i64 pos = get_line_end_pos(app, buffer_id, line_num);
    Rect_f32 text_layout_rect = text_layout_character_on_screen(app, text_layout_id, pos);
    Vec2_f32 draw_position  = {text_layout_rect.x0 + 10, text_layout_rect.y0};
    
    char path_string[100];
    g_debugger_info.process.GetSelectedThread().GetFrameAtIndex(0).GetLineEntry().GetFileSpec().GetPath(path_string, sizeof(path_string));
    String_Const_u8 buffer_file_name = push_buffer_file_name(app, scratch, buffer_id);
    //TODO(wormboy): not working properly
    fprintf(stderr, "%d %s %s\n", buffer_id, path_string, (const char*)buffer_file_name.str);
    if (strcmp(path_string, (const char*)buffer_file_name.str) == 0) {
        draw_string(app, face_id, string_to_draw, draw_position, finalize_color(defcolor_comment, 0));
    }
#endif
}

void debugger_draw_breakpoint_annotation(Application_Links *app, Buffer_ID buffer_id, Text_Layout_ID text_layout_id, Face_ID face_id) {
    Scratch_Block scratch(app);
    String_Const_u8 buffer_file_name = push_buffer_file_name(app, scratch, buffer_id);
    
    int current_line_num = g_debugger_info.process.GetSelectedThread().GetFrameAtIndex(0).GetLineEntry().GetLine();
    char path_string[100];
    g_debugger_info.process.GetSelectedThread().GetFrameAtIndex(0).GetLineEntry().GetFileSpec().GetPath(path_string, sizeof(path_string));
    b32 draw_current_line = false;
    if (strcmp(path_string, (const char*)buffer_file_name.str) == 0) {
        draw_current_line = true;
    }
    
    String_Const_u8 breakpoint_string = push_stringf(scratch, "%s", "<=breakpoint");
    String_Const_u8 current_line_string = push_stringf(scratch, "%s", "<=current line");
    String_Const_u8 breakpoint_current_line_string = push_stringf(scratch, "%s", "<=breakpoint <= current line");
    
    
    b32 has_drawn_current_line = false;
    for (int i = 0; i < g_debugger_info.target.GetNumBreakpoints(); ++i) {
        debugger_breakpoint breakpoint = g_debugger_info.breakpoints[i];
        //fprintf(stderr, "%s, %s\n", (const char*)buffer_file_name.str, breakpoint.breakpoint_location_file_name);
        i64 pos = get_line_end_pos(app, buffer_id, breakpoint.line_num);
        Rect_f32 text_layout_rect = text_layout_character_on_screen(app, text_layout_id, pos);
        Vec2_f32 draw_position  = {text_layout_rect.x0 + 10, text_layout_rect.y0};
        
        if (strcmp(breakpoint.breakpoint_location_file_name, (const char*)buffer_file_name.str) == 0) {
            if (draw_current_line && breakpoint.line_num == current_line_num) {
                draw_string(app, face_id, breakpoint_current_line_string, draw_position, finalize_color(defcolor_comment, 0));
                has_drawn_current_line = true;
            }
            else {
                draw_string(app, face_id, breakpoint_string, draw_position, finalize_color(defcolor_comment, 0));
            }
        }
    }
    
    if (draw_current_line && !has_drawn_current_line) {
        i64 pos = get_line_end_pos(app, buffer_id, current_line_num);
        Rect_f32 text_layout_rect = text_layout_character_on_screen(app, text_layout_id, pos);
        Vec2_f32 draw_position  = {text_layout_rect.x0 + 10, text_layout_rect.y0};
        draw_string(app, face_id, current_line_string, draw_position, finalize_color(defcolor_comment, 0));
    }
    
}

void debugger_jump_to_current_line(Application_Links *app) {
    //TODO(wormboy): open file if not open
    SBLineEntry line_entry = g_debugger_info.process.GetThreadAtIndex(0).GetFrameAtIndex(0).GetLineEntry();
    SBFileSpec file_spec = line_entry.GetFileSpec();
    
    char file_path[100]; //TODO(wormboy): change
    file_spec.GetPath(file_path, sizeof(file_path));
    
    View_ID view_id = get_active_view(app, Access_Always);
    //view_open_file(app, view_id, string_u8_litexpr(file_spec.GetFilename()), true);
    //Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_Always);
    //Buffer_ID buffer_id = get_buffer_by_name(app, string_u8_litexpr(file_spec.GetFilename()), Access_Always);
    Buffer_ID buffer_id = get_buffer_by_name(app, SCu8((u8*)file_spec.GetFilename()), Access_Always);
    view_set_buffer(app, view_id, buffer_id, Access_Always);
    
    Name_Line_Column_Location location;
    location.file = string_u8_litexpr(file_spec.GetFilename());
    location.line = line_entry.GetLine();
    location.column = 0;
    
    //fprintf(stderr, "%s\n", file_spec.GetFilename());
    
    jump_to_location(app, view_id, buffer_id, location);
    
    //if (get_jump_buffer(app, &buffer_id, &location)){
    //jump_to_location(app, view_id, buffer_id, location);
    //fprintf(stderr, "true\n");
    //}
}

void debugger_open_debugger_buffer(Application_Links *app) {
    //TODO:(wormboy) Check if buffer already exsists
    Buffer_Identifier debug_buffer_identifier;
    debug_buffer_identifier.name = debug_buffer_name;
    debug_buffer_identifier.name_len = cstring_length(debug_buffer_name);
    debug_buffer_identifier.id = create_buffer(app, string_u8_litexpr(debug_buffer_name), BufferCreate_AlwaysNew | BufferCreate_NeverAttachToFile);
    
    //TODO:(wormboy) make sure this is the correct view
    View_ID view_id = get_active_view(app, 0);
    view_set_buffer(app, g_view_info.right_view, debug_buffer_identifier.id, 0);
}

bool debugger_begin(Application_Links *app, const char *program_name) {
    SBError error;
    
    error = SBDebugger::InitializeWithErrorHandling();
    
    if (error.Fail()) {
        debugger_print_error( error.GetCString());
        return false ;
    }
    
    g_debugger_info.debugger = SBDebugger::Create();
    g_debugger_info.debugger.SetAsync(false);
    
    if (!g_debugger_info.debugger.IsValid()) {
        debugger_print_error("debugger is not valid");
        return false;
    }
    
    g_debugger_info.target = g_debugger_info.debugger.CreateTarget(program_name, NULL, NULL,
                                                                   true, error);
    
    if (error.Fail()) {
        debugger_print_error( error.GetCString());
        return false;
    }
    
    debugger_open_debugger_buffer(app);
    
    return true;
}

void lldb_end() {
    SBDebugger::Terminate();
}

void debugger_print_breakpoints(Buffer_Insertion *out) {
    insertf(out, "-------------[breakpoints]-------------\n");
    for (int i = 0; i < g_debugger_info.breakpoints_count; ++i) {
        SBStream stream;
        g_debugger_info.breakpoints[i].breakpoint.GetDescription(stream, true);
        insertf(out,"%s\n", stream.GetData());
    }
}

void debugger_begin_printing(Application_Links *app, Buffer_Insertion *out) {
    
}

void debugger_end_printing(Buffer_Insertion *out) {
    
}

function View_ID
get_prev_view_after_active(Application_Links *app, Access_Flag access){
    //NOTE(wormboy): ought to be added to 4coder's default functions
    View_ID view = get_active_view(app, access);
    if (view != 0){
        view = get_prev_view_looped_primary_panels(app, view, access);
    }
    return(view);
}

void debugger_print_thread(Buffer_Insertion *out) {
    SBThread thread = g_debugger_info.process.GetThreadAtIndex(0);
    insertf(out, "-------------[thread]-------------\n");
    //TODO(wormboy): not working properly, something to do with stream?
    SBStream stream;
    thread.GetDescription(stream);
    //fprintf(stderr, "%s\n", stream.GetData());
    insertf(out, "%s", stream.GetData());
}


void debugger_print_disassembly(Buffer_Insertion *out) {
    SBThread thread = g_debugger_info.process.GetThreadAtIndex(0);
    SBFrame frame = thread.GetFrameAtIndex(0);
    insertf(out, "-------------[disassembly]-------------\n");
    insertf(out, "%s", frame.Disassemble());
}

void debugger_print(Application_Links *app) {
    Buffer_Insertion buffer_insertion;
    Scratch_Block scratch(app);
    Cursor insertion_cursor = make_cursor(push_array(scratch, u8, KB(256)), KB(256));
    Buffer_ID buffer_id = get_buffer_by_name(app, SCu8((u8*)debug_buffer_name), 0);
    View_ID view_id = get_prev_view_after_active(app, Access_Always);
    //Buffer_ID buffer_id = create_or_switch_to_buffer_and_clear_by_name(app, SCu8((u8*)debug_buffer_name), view_id);
    
    clear_buffer(app, buffer_id);
    buffer_send_end_signal(app, buffer_id);
    
    buffer_insertion = begin_buffer_insertion_at_buffered(app, buffer_id, 0, &insertion_cursor);
    
    insertf(&buffer_insertion, "-------------[output]-------------\n");
    char stdout_string[100];
    g_debugger_info.process.GetSTDOUT(stdout_string, sizeof(stdout_string));
    
    insertf(&buffer_insertion, "-------------[random]-------------\n");
    SBStream stream;
    g_debugger_info.process.GetSelectedThread().GetFrameAtIndex(0).GetLineEntry().GetDescription(stream);
    insertf(&buffer_insertion, "current line: %s\n", stream.GetData());
    
    char executable_path_name[100];
    g_debugger_info.target.GetExecutable().GetPath(executable_path_name, sizeof(executable_path_name));
    insertf(&buffer_insertion, "executable name: %s\n", executable_path_name);
    
    
    //insertf(&buffer_insertion, "-------------[errors]-------------\n\n");
    
    insertf(&buffer_insertion, "-------------[evaluation]-------------\n");
    fprintf(stderr, "%d\n", g_debugger_info.evaluation_strings_count);
    for (int i = 0; i < g_debugger_info.evaluation_strings_count; ++i) {
        fprintf(stderr, "%s\n", g_debugger_info.evaluation_strings[i]);
        insertf(&buffer_insertion, "%s", g_debugger_info.evaluation_strings[i]);
    }
    
    debugger_print_breakpoints(&buffer_insertion);
    
    debugger_print_thread(&buffer_insertion);
    
    //debugger_begin_printing(app, &buffer_insertion);
    
    //insertf(buffer_insertion, "current line", i, frame.GetDisplayFunctionName());
    
    debugger_print_backtrace(&buffer_insertion);
    
    debugger_print_variables(&buffer_insertion);
    
    debugger_print_registers(&buffer_insertion);
    
    debugger_print_disassembly(&buffer_insertion);
    
    //debugger_end_printing(&buffer_insertion);
    
    end_buffer_insertion(&buffer_insertion);
}

void debugger_add_breakpoint_on_current_line(Application_Links *app) {
    i32 line_num = debugger_get_current_line(app);
    debugger_add_breakpoint(app, line_num);
    debugger_print(app);
}

void debugger_run(Application_Links *app) {
    SBError error;
    
    SBLaunchInfo launch_info = g_debugger_info.target.GetLaunchInfo();
    //g_debugger_info.debugger.SetAsync(false);
    //launch_info.SetWorkingDirectory("/home/wormboy/lldb_project");
    //debugger.SetAsync(true);
    launch_info.SetLaunchFlags(launch_info.GetLaunchFlags() | eLaunchFlagDebug );
    
    //TODO(wormboy): I dont like this line, add an 'if process == not valid" or something
    //g_debugger_info.target.GetProcess().Kill();
    
    g_debugger_info.process = g_debugger_info.target.Launch(launch_info, error);
    //g_debugger_info.process = g_debugger_info.target.LaunchSimple(NULL, NULL, NULL);
    SBListener listener =g_debugger_info.debugger.GetListener();
    //g_debugger_info.process = g_debugger_info.target.Launch(listener, NULL, NULL, NULL, NULL, NULL, NULL, 0, true, error);
    
    if (error.Fail()) {
        debugger_print_error( error.GetCString());
        return;
    }
    
    //g_debugger_info.breakpoints_count = 0;
    
    debugger_print(app);
    
    g_debugger_info.is_running = true;
    
}

void debugger_step_into(Application_Links *app) {
#if 1
    //TODO(wormboy): find the proper step into function that accepts an error parameter$
    SBError error;
    g_debugger_info.process.GetThreadAtIndex(0).StepInto();
    
    debugger_print(app);
    
    debugger_jump_to_current_line(app);
#endif
}

void debugger_step_over(Application_Links *app) {
    SBError error;
    g_debugger_info.process.GetThreadAtIndex(0).StepOver(eOnlyDuringStepping, error);
    
    if (error.Fail()) {
        debugger_print_error( error.GetCString());
        return;
    }
    
    debugger_jump_to_current_line(app);
    
    debugger_print(app);
}

void debugger_jump_to_line_disassembly(Application_Links *app) {
    //NOTE(wormboy): not working, use disassemble instead
    SBLineEntry line;
    line.SetFileSpec(g_debugger_info.target.GetExecutable());
    line.SetLine(debugger_get_current_line(app));
    
    SBStream streamy;
    line.GetDescription(streamy);
    fprintf(stderr, "debugger jump to line:%s\n", streamy.GetData());
    
    SBAddress line_address = line.GetStartAddress();
    
    line_address.GetDescription(streamy);
    fprintf(stderr, "%s\n", streamy.GetData());
    
    SBInstructionList instruction_list = g_debugger_info.target.ReadInstructions(line_address, 10);
    Assert(instruction_list.IsValid());
    
    SBStream stream;
    instruction_list.GetDescription(stream);
    //fprintf(stderr, "%s\n", stream.GetData());
}

void do_somthing() {
#if 1
#if 1
    SBLineEntry line =  g_debugger_info.process.GetThreadAtIndex(0).GetFrameAtIndex(0).GetLineEntry();
    //line.SetLine(22);
    //line = g_debugger_info.process.GetThreadAtIndex(0).GetFrameAtIndex(0).GetLineEntry();
    //SBAddress line_address = line.GetStartAddress();
    //SBAddress line_address = g_debugger_info.target.ResolveFileAddress(22);
    SBAddress line_address =  g_debugger_info.process.GetThreadAtIndex(0).GetFrameAtIndex(0).GetPCAddress();
    
    SBStream streamess;
    line.GetDescription(streamess);
    //fprintf(stderr, "line: %s\n", streamess.GetData());
    
    SBStream stream10;
    line.GetStartAddress().GetDescription(stream10);
    //fprintf(stderr, "lineasdasd: %s\n", stream10.GetData());
    
    SBStream address_stream;
    line_address.GetDescription(address_stream);
    //fprintf(stderr, "address line: %s\n", address_stream.GetData());
#endif
#if 0
    SBAddress address(0x401160, g_debugger_info.target);
    SBStream stream;
    address.GetDescription(stream);
    fprintf(stderr, "addresss info: %s\n", stream.GetData());
    
    SBAddress address2(0x4011ed, g_debugger_info.target);
    SBStream stream3;
    address2.GetDescription(stream3);
    fprintf(stderr, "address2s info: %s\n", stream3.GetData());
#endif
    
    SBInstructionList instruction_list = g_debugger_info.target.ReadInstructions(line_address, 10);
    Assert(instruction_list.IsValid());
    SBStream stream2;
    instruction_list.GetDescription(stream2);
    //fprintf(stderr, "%s\n", stream2.GetData());
    //SBSymbol symbol = address.GetSymbol();
#if 0 
    for (int i = 0; i < 10; ++i ) {
        SBStream stream;
        SBInstruction instruction = instruction_list.GetInstructionAtIndex(i);
        instruction.Print(stderr);
        
    }
#endif
    //fprintf(stderr, "symbol name: %s\n", symbol.GetName());
#endif
}


void debugger_continue(Application_Links *app) {
    SBError error = g_debugger_info.process.Continue();
    
    if (error.Fail()) {
        debugger_print_error( error.GetCString());
        return;
    }
    
    debugger_print(app);
}

void debugger_delete_all_breakpoints() {
    g_debugger_info.target.DeleteAllBreakpoints();
    g_debugger_info.breakpoints_count = 0;
}

void debugger_query_evaluate_expression(Application_Links *app) {
    char evaluation_string[100];
    get_query_string(app, "Evaluate: ", (u8 *)evaluation_string, sizeof(evaluation_string));
    
    //SBValue value = g_debugger_info.target.EvaluateExpression(evaluation_string);
    SBValue value = g_debugger_info.process.GetThreadAtIndex(0).GetFrameAtIndex(0).EvaluateExpression(evaluation_string);
    
    SBStream stream;
    value.GetDescription(stream);
    
    sprintf(g_debugger_info.evaluation_strings[g_debugger_info.evaluation_strings_count++], "%s  =>  %s", evaluation_string, stream.GetData());
    
    //strcpy(g_debugger_info.evaluation_strings[g_debugger_info.evaluation_strings_count++], stream.GetData());
    
    debugger_print(app);
    
    fprintf(stderr, "%s", stream.GetData());
}

void debugger_interactive_open_executable(Application_Links *app, char *executable_name_out) {
    for (;;) {
        Scratch_Block scratch(app);
        //TODO(wormboy): change this to accomidate different user's views
        View_ID view = g_view_info.bottom_view;
        File_Name_Result result = get_file_name_from_user(app, scratch, "Executable:", view);
        if (result.canceled) break;
        
        String_Const_u8 file_name = result.file_name_activated;
        if (file_name.size == 0) {
            //TODO(wormboy): report error
            break;
        }
        
        //TODO(wormboy): investigate this weired naming of strings
        String_Const_u8 path;
        path = result.path_in_text_field;
        //fprintf(stderr, "%s %s\n", path.str, file_name.str);
        String_Const_u8 full_file_name =
            push_u8_stringf(scratch, "%.*s/%.*s",
                            string_expand(path), string_expand(file_name));
        
        sprintf(executable_name_out, "%s", full_file_name.str);
        
        
        if (result.is_folder){
            set_hot_directory(app, full_file_name);
            continue;
        }
        
        if (character_is_slash(file_name.str[file_name.size - 1])){
            File_Attributes attribs = system_quick_file_attributes(scratch, full_file_name);
            if (HasFlag(attribs.flags, FileAttribute_IsDirectory)){
                set_hot_directory(app, full_file_name);
                continue;
            }
            if (query_create_folder(app, file_name)){
                set_hot_directory(app, full_file_name);
                continue;
            }
            break;
        }
        
#if 0
        Buffer_Create_Flag flags = BufferCreate_NeverNew;
        Buffer_ID buffer = create_buffer(app, full_file_name, flags);
        if (buffer != 0){
            view_set_buffer(app, view, buffer, 0);
        }
#endif
        break;
    }
}

void debugger_begin_with_query_executable(Application_Links *app) {
#if 0
    char executable_string[100];
    get_query_string(app, "Executable: ", (u8 *)executable_string, sizeof(executable_string));
    fprintf(stderr, "executable_string %s\n", executable_string);
#endif
    
    char executable_name[100];
    debugger_interactive_open_executable(app, executable_name);
    //fprintf(stderr, "%s\n", executable_name);
    
    //fprintf(stderr, "size: %ld string: %s\n", query_bar->string.size, (const char *)query_bar->string.str);
    
    debugger_begin(app, executable_name);
}

void debugger_quick_start(Application_Links *app) {
#if 0
    get_query_string(app, "Executable: ", (u8 *)executable_string, sizeof(executable_string));
    bool begun_successfuly = debugger_begin(app, executable_string);
    if (!begun_successfuly) {
        return;
    }
#endif
}

void debugger_set_watchpoint_on_identifier() {
    
}

void debugger_set_breakpoint_on_identifier() {
    
}

void debugger_set_breakpoint_on_function_interactive(Application_Links *app) {
    
}

void debugger_test(Application_Links *app) {
    
    char executable_string[100];
    //get_query_string(app, "Executable: ", (u8 *)executable_string, sizeof(executable_string));
    
    //debugger_begin(app, "~/some_program");
    
    //bool begun_successfuly = debugger_begin(app, executable_string);
    bool begun_successfuly = debugger_begin(app, "~/some_program");
    if (!begun_successfuly) {
        return;
    }
    
    Buffer_ID buffer_id;
    if (!open_file(app, &buffer_id, string_u8_litexpr("/home/wormboy/some_program.c"), false, true))
    {
        debugger_print_error( "failed to load file");
        return;
    }
    
    view_set_buffer(app, g_view_info.left_view, buffer_id, 0);
    
    debugger_delete_all_breakpoints();
    
    debugger_add_breakpoint_on_buffer(app, 17, buffer_id);
    debugger_add_breakpoint_on_buffer(app, 22, buffer_id);
    
    debugger_run(app);
    
    //fprintf(stderr, "%d\n", g_debugger_info.target.GetNumModules());
#if 1
    for (int i = 0; i  < g_debugger_info.target.GetNumModules(); ++i) {
        SBStream stream;
        g_debugger_info.target.GetModuleAtIndex(i).GetDescription(stream);
        fprintf(stderr, "%s\n", stream.GetData());
    }
#endif
    
    SBThread thread = g_debugger_info.process.GetThreadAtIndex(0);
    
    if (!thread.IsValid()) {
        debugger_print_error( "thread is not valid");
    }
    
    //NOTE(wormboy): a proper breakpoint must be set for this to work
    SBFrame frame = thread.GetFrameAtIndex(0);
    
    if (!frame.IsValid()) {
        debugger_print_error( "frame not valid");
        return;
    }
    
#if 0
    SBStream stream;
    thread.GetDescription(stream);
    //ERROR: stack smashing?!
    char str[500];
    sprintf(str, "thread data: %s", stream.GetData());
    buffer_insert(app, str);
#endif
    
    do_somthing();
    //debugger_step_over(app);
    //debugger_print(app);
}

//4CODER COMMANDS//
CUSTOM_COMMAND_SIG(command_debugger_quick_start)
CUSTOM_DOC("debugger quick start") {
    debugger_quick_start(app);
}

CUSTOM_COMMAND_SIG(command_debugger_query_evaluate_expression)
CUSTOM_DOC("debugger evaluate") {
    debugger_query_evaluate_expression(app);
}

CUSTOM_COMMAND_SIG(command_debugger_begin_with_query_executable)
CUSTOM_DOC("debugger begin and query the user for executable name") {
    debugger_begin_with_query_executable(app);
}

CUSTOM_COMMAND_SIG(command_debugger_begin)
CUSTOM_DOC("debugger begin") {
    //debugger_begin(app);
}

CUSTOM_COMMAND_SIG(command_debugger_break_on_current_line_and_run)
CUSTOM_DOC("debugger break at current line and run") {
    debugger_delete_all_breakpoints();
    debugger_add_breakpoint_on_current_line(app);
    debugger_run(app);
}

CUSTOM_COMMAND_SIG(command_debugger_step_over)
CUSTOM_DOC("debugger step over") {
    debugger_step_over(app);
}

CUSTOM_COMMAND_SIG(command_debugger_step_into)
CUSTOM_DOC("debugger step into") {
    debugger_step_into(app);
}

CUSTOM_COMMAND_SIG(command_debugger_run)
CUSTOM_DOC("run debugger") {
    debugger_run(app);
}

#if 0
CUSTOM_COMMAND_SIG(command_debugger_jump_to_line_disassembly)
CUSTOM_DOC("debugger_jump_to_line_disassembly") {
    debugger_jump_to_line_disassembly(app);
}
#endif

CUSTOM_COMMAND_SIG(command_debugger_add_breakpoint_on_current_line)
CUSTOM_DOC("add breakpoint") {
    debugger_add_breakpoint_on_current_line(app);
}

CUSTOM_COMMAND_SIG(command_debugger_test)
CUSTOM_DOC("begin debugger session") {
    debugger_test(app);
}

CUSTOM_COMMAND_SIG(command_debugger_continue)
CUSTOM_DOC("continue") {
    debugger_continue(app);
}

#if 0
CUSTOM_COMMAND_SIG(process_launch)
CUSTOM_DOC("begin debugger session") {
    lldb_process_launch();
}

CUSTOM_COMMAND_SIG(debugger_begin)
CUSTOM_DOC("begin debugger session") {
    lldb_begin(app);
}

CUSTOM_COMMAND_SIG(debugger_end)
CUSTOM_DOC("end debugging session") {
    lldb_end();
}
#endif