# OROCOS dot service

_This branch is meant to be used in the context of the rtt_ros2 integration._

The rtt_dot_service is an RTT service which generates a file in the [DOT language format]("http://www.graphviz.org/doc/info/lang.html") containing an overview of your current deployment configuration.
 It can be visualised with any DOT visualizer to give you an overview of:
 
    - all components currently deployed, together with their status information.
    - all component ports (connected and unconnected), and how they are connected.

The service takes into account all peer components of the component in which you load the service. To get an overview of your complete deployment configuration, load this service in the Deployer component. 
You can trigger the execution: manually, using the `generate()` function, but it will execute automatically with every component update as well (don't forget to attach an activity to your Deployer component!)

To use it, load the service in your Deployer component, e.g. in your .ops script, add:
```
import("rtt_ros2")
ros.import("rtt_dot_service")
loadService("Deployer","dot")
```
or equivalently in a .lua script:

```
rtt.provides("ros"):import("rtt_dot_service")
depl:loadService("Deployer","dot")
```

The service has a property, `dot_file`, which you can adjust to the file you like to be generated. The way components, connections and channels can be plotted can be tuned using the comp_args, conn_args and chan_args properties. Visualisation of the dot file is possible, e.g. with  xdot:
```
xdot orograph.dot
```

Colors are used to display the component's current state:

   - Init - **white**
   - PreOperational - **orange**
   - FatalError, Exception, RunTimeError - **red**
   - Stopped - **lightblue** 
   - Running - **green**


More information about the DOT language is available at http://www.graphviz.org/doc/info/lang.html and http://www.graphviz.org/Documentation/dotguide.pdf
