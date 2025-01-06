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
- **Mutex:** ❌
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

## License

Taskio is distributed under the [MIT License](https://opensource.org/licenses/MIT).

## Acknowledgements

- **Tokio**: For the inspiration behind Taskio's development. - [tokio.rs](https://tokio.rs)
- **Writing an OS in Rust: Async/Await** - [os.phil-opp.com/async-await/](https://os.phil-opp.com/async-await/)
