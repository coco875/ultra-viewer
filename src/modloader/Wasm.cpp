// #include <cstdint>
// #include <string>
// #include <iostream>
// #include <vector>
// #include <filesystem>
// #include <wasm_export.h>
// #include <wasm_exec_env.h>

// struct mod_instance {
//     wasm_exec_env_t exec_env;
//     char* name;
// };

// std::vector<mod_instance> mods_instance = {};

// bool CreateDirectoryRecursive(std::string const& dirName, std::error_code& err) {
//     err.clear();
//     if (!std::filesystem::create_directories(dirName, err)) {
//         if (std::filesystem::exists(dirName)) {
//             // The folder already exists:
//             err.clear();
//             return true;
//         }
//         return false;
//     }
//     return true;
// }

// void load_mod_wasm_file(char* mod_name, char* buffer, size_t size) {
//     char error_buf[128];
//     wasm_module_t module;
//     wasm_module_inst_t module_inst;
//     wasm_exec_env_t exec_env;
//     uint32_t stack_size = 8092 * 8092, heap_size = 8092 * 8;
//     /* parse the WASM file from buffer and create a WASM module */
//     module = wasm_runtime_load((uint8_t*) buffer, size, error_buf, sizeof(error_buf));
//     printf("error? %s\n", error_buf);

//     if (module == NULL) {
//         printf("%s\n", error_buf);
//         exit(-1);
//     }

//     /* create an instance of the WASM module (WASM linear memory is ready) */
//     module_inst = wasm_runtime_instantiate(module, stack_size, heap_size, error_buf, sizeof(error_buf));
//     if (module_inst == NULL) {
//         printf("%s\n", error_buf);
//         exit(-1);
//     }

//     /* lookup a WASM function by its name
//      The function signature can NULL here */

//     /* creat an execution environment to execute the WASM functions */
//     exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
//     mods_instance.push_back({ exec_env, mod_name });
// }

// mod_instance* find_mod(char* name) {
//     for (size_t i = 0; i < mods_instance.size(); i++) {
//         if (strcmp(mods_instance[i].name, name) == 0) {
//             return &mods_instance[i];
//         }
//     }
//     return NULL;
// }

// wasm_module_inst_t get_mods_instance(char* mod_name) {
//     return wasm_exec_env_get_module_inst(find_mod(mod_name)->exec_env);
// }

// wasm_function_inst_t mod_lookup_function(char* mod_name, char* function_name) {
//     return wasm_runtime_lookup_function(get_mods_instance(mod_name), function_name);
// }

// bool mod_call_function_wasm(char* mod_name, char* function_name, uint32 argc, uint32 argv[]) {
//     wasm_function_inst_t func = mod_lookup_function(mod_name, function_name);
//     if (func == NULL) {
//         printf("error to find the function\n");
//         return false;
//     }

//     return wasm_runtime_call_wasm(find_mod(mod_name)->exec_env, func, 1, argv);
// }

// uint32 call_extern_function(wasm_exec_env_t exec_env, char* mod_name, char* function_name, uint32 argc,
//                             uint32 argv_offset) {
//     if (!wasm_runtime_validate_app_addr(wasm_exec_env_get_module_inst(exec_env), (uint64) argv_offset,
//                                         (uint64) argc * 4)) {
//         return false;
//     }
//     uint32* argv =
//         (uint32*) wasm_runtime_addr_app_to_native(wasm_exec_env_get_module_inst(exec_env), (uint64) argv_offset);
//     return mod_call_function_wasm(mod_name, function_name, argc, argv);
// }

// /* the native functions that will be exported to WASM app */
// static NativeSymbol native_symbols[] = {
//     EXPORT_WASM_API_WITH_SIG(call_extern_function, "($$ii)i"),
//     // EXPORT_WASM_API_WITH_SIG(display_input_read, "(*)i"),
//     // EXPORT_WASM_API_WITH_SIG(display_flush, "(iiii*)")
// };

// static char global_heap_buf[512 * 1024];

void load_mods() {
//     wasm_runtime_init();

//     RuntimeInitArgs init_args;
//     memset(&init_args, 0, sizeof(RuntimeInitArgs));

//     /* configure the memory allocator for the runtime */
//     init_args.mem_alloc_type = Alloc_With_Pool;
//     init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
//     init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

//     /* set maximum thread number if needed when multi-thread is enabled,
//     the default value is 4 */
//     init_args.max_thread_num = 4;

//     /* initialize runtime environment with user configurations*/
//     if (!wasm_runtime_full_init(&init_args)) {
//         exit(-1);
//     }

//     int n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
//     if (!wasm_runtime_register_natives("env", native_symbols, n_native_symbols)) {
//         exit(-1);
//     }

//     std::error_code err;
//     if (!CreateDirectoryRecursive("mods", err)) {
//         // Report the error:
//         std::cout << "CreateDirectoryRecursive FAILED, err: " << err.message() << std::endl;
//     }
}