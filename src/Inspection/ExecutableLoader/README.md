# Architecture

## Basic functionality

* Run executable
```cpp
    // Basic form
    ExecutableLoader loader;
    loader.setFileName("executable1.exe");
    loader.setArgs(std::vector<std::string>{"file1.txt", "open_mode=read", "--gui"});
    loader.run();

    // compact form 1 (maybe?)
    ExecutableLoader loader2;
    loader2.setFileName("executable1.exe");
    loader.run(std::vector<std::string>{"file1.txt", "open_mode=read", "--gui"});

    // compact form 2 (maybe?)
    ExecutableLoader loader2("executable1.exe");
    loader.run(std::vector<std::string>{"file1.txt", "open_mode=read", "--gui"});
```

* Close executable
```cpp
    int exitCode = loader.exit();
```

* Wait for exit
```cpp
    loader.waitForExit();
```

* Get exit code
```cpp
    loader.run();
    loader.waitForExit();
    return loader.exitCode();
```

* Register a loader
```cpp
    // Not totally sure about the location of such API yet, but most likely it would be in the loader
    PM::registerExecutableLoader<CustomExecutableLoader>("CustomExecutableLoader");
```

## Implementation details

The best implementation would be something that allows the user to supply different loaders with different loading techniques such as:
* Loading the executable as a suspended process then injecting a dll into it before it resumes.
* Loading a process under a debugger
* Creating a dll out of the executable and then injecting it into the client executable
* Implementing a custom windows loader that loads an executable file as it is into the memory of the client process, then manually fills its import table addresses with the required dependencies.

all these implementations should be possible under one simple interface and the loader application should be able to use all of them equally.

We also need to have signals that would be called when the executable starts running, exits or terminates.

Since we have signals and slots, there is no need for having sync / async versions of the class member functions, by default all functions should be non-blocking. This frees the user from spawning multiple threads and the need to handle race conditions. This can be achieved through using `std::async`.

We also need to have a simple way to send additional parameters to the loader. this should be implemented in a way that allows each loader to return a list of all parameters alongside their description and their default values, so that they can be displayed in the UI for the user to modify them at runtime regardless of the currently selected loader.</br>
Such system would be very handy when we need to change something like the default client lib, or if the user wanted to inject some additional library or modify the injection method at runtime through the UI, configuration files, application settings or even through code. 

## Nice-to-haves

| Feature | Description | Priority |
| --- | --- | --- |
