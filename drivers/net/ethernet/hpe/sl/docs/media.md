# Media

# Table of Contents

- [Table of Contents](#table-of-contents)
  - [LED States](#llr-states)
    - [State Description](#state-description)

## LED States
A bi-color status LED driven by the sC to indicate link status for the port(s) connected to the cable.

### State Description

The table below displays the LED status for the jack, which reflects the combined link states of the ports.
LED status is prioritized from top to bottom.

| **Cable**        | **Link Condition**      | **LED Color**        |
|------------------|-------------------------|----------------------|
| Unplugged        | N/A                     | OFF                  |
| Plugged          | High temperature        | AMBER ON SOLID       |
| Plugged          | Media error             | AMBER FAST BLINK     |
| Plugged          | Link starting           | GREEN FAST BLINK     |
| Plugged          | Link up                 | GREEN ON SOLID       |
| Plugged          | All others              | OFF                  |

