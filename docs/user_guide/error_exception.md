# Error && Exception

## Error
Zedio use [std::exception](https://en.cppreference.com/w/cpp/error/exception) to wrap function return value or error.

## Exception
- DEBUG：No matter where the exception comes from, the program will be terminated
- RELEASE：If the exception is not from the main coroutine, zedio simply outputs an exception message to std::cerr