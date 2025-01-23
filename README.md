# Taskio

Taskio is an ongoing project, aiming to be an lazy asynchronous runtime library for C23, providing utilities for building asynchronous applications, and inspired by Tokio.

## Features

### Platform Support
| Platform | Windows | Linux | MacOS |
|----------|:-------:|:-----:|:-----:|
| Support  |    ❌    |   ✅  |   ❌   |

### Runtime
- **Single Threaded:** ✅
- **Multi-Threaded:** ❌
- **RNG:** ❌
- **Hashed Timing Wheel** ✅

### Task
- **Awaitable:** ✅
- **Abortable:** ✅

### Time
- **Sleep** ✅
- **Timeout** ❌
- **Interval** ❌

### Synchronization Primitives
- **Semaphore:** ✅
- **Mutex:** ✅
- **RwLock:** ❌
- **Notify:** ❌
- **Barrier:** ❌
- **OneShot Channel:** ❌
- **Multi-Producer Single Consumer Channel:** ❌
- **Broadcast Channel:** ❌
- **Watch Channel:** ❌

### File System
- **Read:** ❌
- **Write:** ❌

### Network
- **TCP:** ❌
- **UDP:** ❌

## Hello World in Taskio

```c
#include <stdio.h>
#include <taskio.h>

// environment used by your async main
struct taskio_main_env {
    int argc;
    char** args;
};

// entry point for your async main
taskio_main() {
    // scope to handle a single yield, suspended_yield, await*
    // and async_return at the end
    async_scope() {
        // print hello world
        printf("Hello World\n");
        // finish the async main
        async_return();
    }
}
```

# Await in Taskio

```c
#include <stdio.h>
#include <taskio.h>
#include <taskio/common.h>

// environment used by your async main
struct taskio_main_env {
    int argc;
    char** args;

    // any future that async main await_fn for
    future_env(taskio_sleep);
};

// entry point for your async main
taskio_main() {
    // scope to handle a single yield, suspended_yield, await*
    // and async_return at the end
    async_scope() {
        printf("Main will sleep for 1 second\n");

        // call taskio_sleep(1000) and await for it
        //
        // taskio_sleep is an async function which suspends
        // the async function for some milliseconds of granularity
        await_fn(taskio_sleep(1000));
    }

    // once taskio_sleep(1000) finishes, our async main will
    // come back to continue their work
    async_scope() {
        printf("Main slept for 1 second\n");
        // finish the async main
        async_return();
    }
}
```

## License

Taskio is distributed under the [MIT License](https://opensource.org/licenses/MIT).

## Acknowledgements

- **Tokio**: For the inspiration behind Taskio's development. - [tokio.rs](https://tokio.rs)
- **Writing an OS in Rust: Async/Await** - [os.phil-opp.com/async-await/](https://os.phil-opp.com/async-await/)
