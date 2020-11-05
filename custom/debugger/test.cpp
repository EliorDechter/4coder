#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "lldb/API/LLDB.h"

using namespace lldb;

#if 0
void run_command(const char *command, SBCommandInterperter *interpreter) {
    assert(command);
    
    SBCommandRreturnObject command_return_object;
    interpreter.HandleCommand(command, command_return_object);
    
    const char *output = command_return_object.GetOutput();
    assert(output);
    
}
#endif

void print_variables(SBFrame *frame) {
    SBValueList variables_value_list = frame->GetVariables(true, true, true ,true);
    printf("variables:\n");
    for (uint32_t i = 0; i < variables_value_list.GetSize(); ++i) {
    	SBValue value = variables_value_list.GetValueAtIndex(i);
    	printf("value: %s type: %s\n", value.GetName(), value.GetTypeName());
    }
}

void print_registers(SBFrame *frame) {
    SBValueList registers_value_list = frame->GetRegisters();
    printf("registers:\n");
    for (uint32_t i = 0; i < registers_value_list.GetSize(); ++i) {
        SBValue value = registers_value_list.GetValueAtIndex(i);
        printf("%s\n------------------------\n", value.GetName());
        for (uint32_t i = 0; i < value.GetNumChildren(); ++i) {
            SBValue register_value = value.GetChildAtIndex(i);
            printf("%s %s\n", register_value.GetName(), register_value.GetValue());
        }
    }
}

void print_stacktrace(SBThread *thread) {
#if 0 
    SBFileSpec spec = frame.GetLineEntry().GetFileSpec();
    cosnt char *file_name = spec.GetFileName();
    const char *directory_name = spec.GetDirectory();
#endif
    
    for (uint32_t i = 0; i < thread->GetNumFrames(); ++i) {
        SBFrame frame = thread->GetFrameAtIndex(i);
        printf("frame #%d: %s\n", i, frame.GetDisplayFunctionName());
    }
}

void set_breakpoint(SBTarget *target) {
    SBBreakpoint breakpoint = target->BreakpointCreateByName("foo");
}

void init_lldb() {
}

void deinit_lldb() {
}


int main() {
    SBError error;
    
    SBDebugger::InitializeWithErrorHandling();
    
    SBDebugger debugger(SBDebugger::Create());
    const char *exe_file_path = "/home/4coder/4ed";
    SBTarget target = debugger.CreateTarget(exe_file_path, NULL, NULL,
                                            true, error);
    
    SBLaunchInfo launch_info = target.GetLaunchInfo();
    debugger.SetAsync(false);
    launch_info.SetWorkingDirectory("/home/wormboy/lldb_project");
    //debugger.SetAsync(true);
    //launch_info.SetLaunchFlags(launch_info.GetLaunchFlags() | eLaunchFlagDebug );
    printf("here");
    SBProcess process = target.Launch(launch_info, error);
    SBThread thread = process.GetThreadAtIndex(0);
    SBFrame frame = thread.GetFrameAtIndex(0);
    SBStream stream;
    thread.GetDescription(stream);
    printf("thread data: %s", stream.GetData());
    //printf("hitcount: %d\n", breakpoint.GetHitCount());
    
    //printf("-------------\n");
    
  
#if 0
    //take input
    char input[10] = "";
    while(strcmp(input, "q") != 0) {
        scanf("%s", input); //unsafe
        SBCommandReturnObject command_return_object;
        interpreter.HandleCommand(input, &command_return_object);
#if 0
        if (strcmp(input, "r") == 0) {
            
        }
        if (strcmp(input, "b") == 0) {
            
        }
#endif
    }
#endif
    
    SBDebugger::Terminate();
    
}
