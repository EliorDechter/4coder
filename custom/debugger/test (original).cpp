
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>



#include "lldb/API/LLDB.h"


#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
//#include "nuklear.h"

#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

using namespace lldb;

void callstack_print() {
    
}

int main() {
    SBError errory = SBDebugger::InitializeWithErrorHandling();
    printf("%s\n", errory.GetCString());
    SBDebugger debugger(SBDebugger::Create());
    
    if (!debugger.IsValid()) {
    	fprintf(stderr, "error: failed to create a debugger object\n");
    }
    
    const char *arch = NULL;
    const char *platform = NULL;
    
    const bool add_dependent_libs = false;
    SBError error;
    
    const char *exe_file_path = "tasty";
    
    SBTarget target = debugger.CreateTarget(exe_file_path, arch, platform,
                                            true, error);
    assert(target.IsValid());
    
    SBBreakpoint breakpoint = target.BreakpointCreateByName("fucker");
    //printf("%zu heeloo\n", breakpoint.GetNumLocations());
    //assert(breakpoint.IsEnabled());
    //SBBreakpoint breakpoint = target.BreakpointCreateByLocation("tasty.c", 4);
    //assert(breakpoint.IsValid());
    //const char *flags = "hello";
    //SBLaunchInfo launch_info(&flags);
    //launch_info.SetLaunchFlags(eLaunchFlagStopAtEntry);
    SBError error2;
    SBLaunchInfo launch_info = target.GetLaunchInfo();
    //launch_info.SetLaunchFlags(eLaunchFlagStopAtEntry);
    
    SBProcess process = target.Launch(launch_info, error2);
    printf("eror: %s\n", error.GetCString());
    //SBProcess process = target.LaunchSimple(NULL, NULL, target.GetPlatform().GetWorkingDirectory());
    assert(process.IsValid());
    
    SBThread thread = process.GetThreadAtIndex(0);
    assert(thread.IsValid());
    SBFrame frame = thread.GetFrameAtIndex(0);
    assert(frame.IsValid());
    
    SBStream stream;
    thread.GetDescription(stream);
    printf("%s\n", stream.GetData());
    
    printf("%d\n", thread.GetStopReason());
    
    
#if 0
    printf("name: %s\n", frame.GetFunction().GetDisplayName());
    
    printf("%d\n", breakpoint.GetHitCount());
#endif
    
    char input_buf[2048];
    thread.GetStopDescription(input_buf, 2048);
    printf("%s\n", input_buf);
    
    assert(!thread.IsStopped());
    assert(!thread.IsSuspended());
    //process.GetDescription(stream);
    //printf("process:\n %s\n", stream.GetData());
    for (int i = 0; i < 10; ++i) {
        process.Continue();
        
        thread.GetDescription(stream);
        printf("%s\n", stream.GetData());
    }
    
    SBDebugger::Terminate();
}
