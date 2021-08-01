# WaterMon

Video: https://www.youtube.com/watch?v=He8_Kl8E-tk

WaterMon is an endeavour to create smart solutions for water resource management and water sanitation problems. Factors related to water managment, sanitation, and hygiene affect humans in many ways. According to UNICEF WASH, 400 million school-aged children a year are infected by intestinal worms, which sap their learning abilities. 

We at WaterMon beileve that using technology to automate water managment and sanitation systems will enable better usage of water. The aim is to create solutions that are affordable, automated, efficient and sustainable.

The method is designed to be crowd-sourced and handled by a local communities to create a global impact enabling them to be stakeholders in environmental management.

We encourage all to build on these solutions, contribute to creating innovative solutions.

Website: www.watermon.org
## Projects
### Contaminant Detection
Biological and Chemical contaminants are the main causes of unclean sanitation of water and we at watermon have developed an automated method for detection of the contaminants and develop reports of any water sample based on type, quantity and rate of spread of contaminant present in the body of water.

#### Device design:  
* Built with easily available low cost components.
* Create enclosure for colour sensor to minimise environmental factors.
* Work without internet to provide results locally and store data for future upload to cloud.
* Interface to enable results monitoring, calibration for each test.

#### Device Features:
* Microcontroller
* Colour Sensor to detect colour change in the ampoule.
* Heating pad to maintain temperature.
* Enclosure with, Borosil beaker.
* UI to configure, calibrate

For more info visit: https://github.com/watermonorg/WaterMon/wiki/Contaminant-Detection

### Leak Detection

Water flow detection is key for inferring leakage and preventing wastage.
Challenges Invasive flow meters although effective require plumbing and maintenance. Flow meters typically have moving spindles which take wear & tear over period. They can accumulate salts and result in calibration drifts.

The intent of this project is to demonstrate a non-invasive water flow detection solution capturing the variations in the vibrations on the pipe during the flow. The first objective is to detect presence of water flow and match it with rules to infer leaks based on the expected duration of water flow.

#### Solution Overview:

* ESP-32 microcontroller with 6-Axis IMU mounted on the Water pipe captures the sensor readings and uploads it to MQTT broker hosted in IBM Cloud
* ML pipeline trains on the IMU sensor readings during the flow and in no flow scenario and generates the coefficients required to classify the flow, no flow scenario
* Node-RED Dashboard uses these coefficients to classify the flow status based on real-time sensor data.

For more info visit : https://github.com/watermonorg/WaterMon/wiki/Leak-detection
