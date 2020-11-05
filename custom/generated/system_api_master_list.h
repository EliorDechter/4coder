api(system) function String_Const_u8 get_path(Arena* arena, System_Path_Code path_code);
api(system) function String_Const_u8 get_canonical(Arena* arena, String_Const_u8 name);
api(system) function File_List get_file_list(Arena* arena, String_Const_u8 directory);
api(system) function File_Attributes quick_file_attributes(Arena* scratch, String_Const_u8 file_name);
api(system) function b32 load_handle(Arena* scratch, char* file_name, Plat_Handle* out);
api(system) function File_Attributes load_attributes(Plat_Handle handle);
api(system) function b32 load_file(Plat_Handle handle, char* buffer, u32 size);
api(system) function b32 load_close(Plat_Handle handle);
api(system) function File_Attributes save_file(Arena* scratch, char* file_name, String_Const_u8 data);
api(system) function b32 load_library(Arena* scratch, String_Const_u8 file_name, System_Library* out);
api(system) function b32 release_library(System_Library handle);
api(system) function Void_Func* get_proc(System_Library handle, char* proc_name);
api(system) function u64 now_time(void);
api(system) function Date_Time now_date_time_universal(void);
api(system) function Date_Time local_date_time_from_universal(Date_Time* date_time);
api(system) function Date_Time universal_date_time_from_local(Date_Time* date_time);
api(system) function Plat_Handle wake_up_timer_create(void);
api(system) function void wake_up_timer_release(Plat_Handle handle);
api(system) function void wake_up_timer_set(Plat_Handle handle, u32 time_milliseconds);
api(system) function void signal_step(u32 code);
api(system) function void sleep(u64 microseconds);
api(system) function String_Const_u8 get_clipboard(Arena* arena, i32 index);
api(system) function void post_clipboard(String_Const_u8 str, i32 index);
api(system) function void set_clipboard_catch_all(b32 enabled);
api(system) function b32 get_clipboard_catch_all(void);
api(system) function b32 cli_call(Arena* scratch, char* path, char* script, CLI_Handles* cli_out);
api(system) function void cli_begin_update(CLI_Handles* cli);
api(system) function b32 cli_update_step(CLI_Handles* cli, char* dest, u32 max, u32* amount);
api(system) function b32 cli_end_update(CLI_Handles* cli);
api(system) function void open_color_picker(Color_Picker* picker);
api(system) function f32 get_screen_scale_factor(void);
api(system) function System_Thread thread_launch(Thread_Function* proc, void* ptr);
api(system) function void thread_join(System_Thread thread);
api(system) function void thread_free(System_Thread thread);
api(system) function i32 thread_get_id(void);
api(system) function void acquire_global_frame_mutex(Thread_Context* tctx);
api(system) function void release_global_frame_mutex(Thread_Context* tctx);
api(system) function System_Mutex mutex_make(void);
api(system) function void mutex_acquire(System_Mutex mutex);
api(system) function void mutex_release(System_Mutex mutex);
api(system) function void mutex_free(System_Mutex mutex);
api(system) function System_Condition_Variable condition_variable_make(void);
api(system) function void condition_variable_wait(System_Condition_Variable cv, System_Mutex mutex);
api(system) function void condition_variable_signal(System_Condition_Variable cv);
api(system) function void condition_variable_free(System_Condition_Variable cv);
api(system) function void* memory_allocate(u64 size, String_Const_u8 location);
api(system) function b32 memory_set_protection(void* ptr, u64 size, u32 flags);
api(system) function void memory_free(void* ptr, u64 size);
api(system) function Memory_Annotation memory_annotation(Arena* arena);
api(system) function void show_mouse_cursor(i32 show);
api(system) function b32 set_fullscreen(b32 full_screen);
api(system) function b32 is_fullscreen(void);
api(system) function Input_Modifier_Set get_keyboard_modifiers(Arena* arena);
api(system) function void set_key_mode(Key_Mode mode);