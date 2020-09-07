Sphere++: Azure Sphere C++ wrapper
==================================

Welcome to the home page of the Sphere++ project.

[Sphere++](https://github.com/mbucchia/sphereplusplus) enables writing elegant
[Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/)
applications in C++. It abstracts the common Azure Sphere concepts using using
object-oriented paradigms.

For example, it encapsulates the concept of file descriptors behind objects:

<table width="100%">
<th>Azure Sphere C API</th>
<tr>
<td>
<pre lang="cpp">
led_fd = GPIO_OpenAsOutput(MT3620_GPIO4,
                           GPIO_OutputMode_PushPull,
                           GPIO_Value_High);
GPIO_SetValue(led_fd, GPIO_Value_High);
sleep(1)
GPIO_SetValue(led_fd, GPIO_Value_Low);
</pre>
</td>
</tr>
</table>

<table width="100%">
<th>Sphere++ wrapper</th>
<tr>
<td>
<pre lang="cpp">
GpioOut led(MT3620_GPIO4,
            GPIO_OutputMode_PushPull);
led.init(true /* Default High */);
led.set(true /* High */);
sleep(1)
led.set(false /* Low */);
</pre>
</td>
</tr>
</table>

Sphere++ also provides a base Application class that encapulates the inner
workings of the event loop, offers convenient features such as timers, and
abstracts high-level functionality like communication with Azure IoT Central.

<table width="100%">
<th>Azure Sphere C timers</th>
<tr>
<td>
<pre lang="cpp">
static void TimerHandler(EventLoopTimer *timer)
{
    ConsumeEventLoopTimerEvent(timer);
    /* Timer do stuff */
}

const struct timespec timerPeriod = {.tv_sec = 1, .tv_nsec = 0};
EventLoopTimer *timer = CreateEventLoopPeriodicTimer(eventLoop,
                                                     &TimerHandler,
                                                     &timerPeriod);
</pre>
</td>
</tr>
</table>

<table width="100%">
<th>Sphere++ timers</th>
<tr>
<td>
<pre lang="cpp">
Timer timer();
timer.init();
timer.connect([]{ /* Lambda do stuff */ });
timer.startPeriodic(1000000);
</pre>
</td>
</tr>
</table>

Sphere++ has no dependencies, it can easily be included into your Azure Sphere
project (by including it as a Git submodule for example) with only [simple
modifications](https://github.com/mbucchia/sphereplusplus/blob/master/README.md)
to your project configuration (`CMakeLists.txt`).

Links
-----

- Repository for the project: [GitHub sphereplusplus](https://github.com/mbucchia/sphereplusplus)
- Repository with sample code: [GitHub sphereplusplus-samples](https://github.com/mbucchia/sphereplusplus-samples)
- Doxygen documentation: [here](doxy/index.html)
