Sphere++: Azure Sphere C++ wrapper
==================================

Welcome to the home page of the Sphere++ project.

[Sphere++](https://github.com/mbucchia/sphereplusplus) enables writing elegant
[Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/)
applications in C++. It abstracts the common Azure Sphere concepts using using
object-oriented paradigms.

For example, it encapsulates the concept of file descriptors behind objects:

```
led_fd = GPIO_OpenAsOutput(MT3620_GPIO4, GPIO_OutputMode_PushPull, GPIO_Value_High);
GPIO_SetValue(led_fd, GPIO_Value_High);
sleep(1)
GPIO_SetValue(led_fd, GPIO_Value_Low);
```

Becomes:

```
GpioOut led(MT3620_GPIO4, GPIO_OutputMode_PushPull);
led.init(true /* High */);
led.set(true /* High */);
sleep(1)
led.set(false /* Low */);
```

Sphere++ also provides a base Application class that encapulates the inner
workings of the event loop, offers convenient features such as timers, and
abstracts high-level functionality like communication with Azure IoT Central.

Sphere++ has no dependencies, it can easily be included into your Azure Sphere
project (by including it as a Git submodule for example) with only [simple
modifications](https://github.com/mbucchia/sphereplusplus/blob/master/README.md)
to your `CMakeLists.txt`.

Links
-----

- Repository for the project: [GitHub sphereplusplus](https://github.com/mbucchia/sphereplusplus)
- Repository with sample code: [GitHub sphereplusplus-samples](https://github.com/mbucchia/sphereplusplus-samples)
- Doxygen documentation: [here](doxy/index.html)
