This is example of sending CRTP velocity commands from AI-deck to crazyflie over CPX.
# Read me first
NOTE: DO NOT RUN THIS WITH PROPELLERS ON CRAZYFLIE, THE VEHICLE WILL FLY IN RANDOM DIRECTION. REMOVE ALL PROPELLERS BEFORE TESTING.

# Pre-requisits

Please Follow getting started with AI-deck guide from bitcraze https://www.bitcraze.io/documentation/tutorials/getting-started-with-aideck/

# building
```
docker run --rm -v ${PWD}:/module aideck-with-autotiler-latest-gap tools/build/make-example . clean model build image
```

Where aideck-with-autotiler-latest-gap is docker provided by bitcraz and updated to latest gap sdk (4.12.0)
# Flashing

Connect AI-deck with JTAG then run:

```
 docker run --rm -v ${PWD}:/module --device /dev/ttyUSB0 --privileged -P aideck-with-autotiler-latest-gap tools/build/make-upload-jtag . flash
```
