# CH552 Power Monitor

A tiny low-cost voltmeter and ammeter by CH552 & INA219. The meter uses INA219 to measure voltage, current, and power. 3 current sensing shunt resistors are used to measure current from 1 mA to 3.2 A.

## Current Sensing Shunt Resistors

To increase measurement accuracy in different ranges, 3 current sensing shunt resistors are used, the VCC voltage drop and the shunt resistor power consumption are also controlled.

| #   |    Resistor | Power Rating |    Tolerance | Package | Current Sensing Range | Voltage Drop      |
| --- | ----------: | -----------: | -----------: | ------- | --------------------- | ----------------- |
| 0   | 0.1 &Omega; |           2W |   &plusmn;1% | 2512    | (80 mA, 3.2 A)        | (0.008 V, 0.32 V) |
| 1   |   1 &Omega; |         1/4W |   &plusmn;1% | 1206    | (8 mA, 100 mA)        | (0.008 V, 0.10 V) |
| 2   |  10 &Omega; |         1/8W | &plusmn;0.1% | 0805    | [1 mA, 10 mA)         | (0.010 V, 0.10 V) |

### Switching Shunt Resistors

P-channel MOSFETs are used to switch the shunt resistors on the high-side, the voltage sampling point is placed between the shunt resistor and the the MOSFET to eliminate the voltage drop on the MOSFET, this is important especially for small shunt resistor like 0.1 &Omega;.

#### Equivalent Shunt Resistor Switching Schematic

![Equivalent Shunt Resistor Switching Schematic](images/Equivalent_Shunt_Resistor_Switching_Schematic.png)

#### Shunt Resistor Switching Strategy

- When powers up, the smallest shunt resistor is switched on to avoid the risk of high current through the circuit.
- If the measured current is out of the range of the current shunt resistor, turn on the shunt resistor next to it first and then turn off the current shunt resistor, by doing this the load will not be interrupted.
- To avoid the shunts switching back and force on a swing current, keep a 20% transition hysteresis.

![Shunt Resistor Transition Hysteresis](images/Shunt_Resistor_Transition_Hysteresis.svg)

#### P-channel MOSFET Selection

- `Vds` should be higher than the maximum voltage to measure.
- `Id` should be higher than the maximum current to measure.
- `Vgs(th)` should be as small as possible, so the MOSFET can saturate in a low supply voltage.
- `Rds(on)` should be small enough to reduce voltage drop to the load.

CJ3401 is selected consider it satisfied the conditions above and its low cost.

## Schematic

![schematic](Hardware/Schematic_CH552-Power-Monitor.png)

## PCB

![PCB Top](Hardware/CH552-Power-Monitor-Top.png)

![PCB Bottom](Hardware/CH552-Power-Monitor-Bottom.png)

## References

- Current Ranger (https://lowpowerlab.com/guide/currentranger)
- CH552-USB-OLED (https://github.com/wagiminator/CH552-USB-OLED)
