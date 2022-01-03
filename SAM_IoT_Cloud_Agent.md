# Using the SAM-IoT as a Cloud Agent for an Application Host Processor

## Introduction

This document describes how to utilize the SAM-IoT WG Development Board as a cloud agent – providing a serial-to-cloud bridge between an application processor (e.g. PIC-IoT Wx Development Board) and an IoT Central application. The SAM-IoT WG Development Board will be provisioned for use with Azure IoT services using self-signed X.509 certificate-based authentication.



## Table of Contents

- [Introduction](#introduction)
- [Background Knowledge](#background-knowledge)
  - [Application Processor Connections to the SAM-IoT Cloud Agent](#Application-Processor-Connections-to-the-SAM-IoT-Cloud-Agent)
  - [Sending Application Processor Telemetry Events](#Sending-Application-Processor-Telemetry-Events)
  - [Reading/Writing Application Processor Properties](#Reading/Writing-Application-Processor-Properties)
  - [Dedicated Telemetry Interface (DTI)](#Dedicated-Telemetry-Interface-(DTI))
  - [Data Frame Format](#Data-Frame-Format)
- [Program the DTI Test Platform using a PIC-IoT Development Board Configured as the SPI Master](#Program-the-DTI-Test-Platform-using-a-PIC-IoT-Development-Board-Configured-as-the-SPI-Master)
  - [Installing the Development Tools](#installing-the-development-tools)
  - [Program SAM-IoT Development Board Configured as SPI Slave](#Program-SAM-IoT-Development-Board-Configured-as-SPI-Slave)
  - [Create an IoT Central Application for your Device](#create-an-iot-central-application-for-your-device)
  - [Validate Connection](#validate-connection)
- [Data Visualization using the IoT Central Application](#Data-visualization-using-the-IoT-Central-Application)
  - [Confirmation of Reception for Every Data Frame Sent by the Application Processor](#Confirmation-of-Reception-for-Every-Data-Frame-Sent-by-the-Application-processor)

    

## Background Knowledge

### Application Processor Connections to the SAM-IoT Cloud Agent

A total of 5 wires need to be connected between the application host processor and the SAM-IoT development board. The SAM-IoT board basically acts as a proxy/bridge between an application processor and the Cloud.  The Command Line Interface (CLI) requires 2 wires for a UART connection and the Dedicated Telemetry Interface (DTI) requires 3 wires for a uni-directional SPI Master-to-Slave connection.  The SPI pins can be easily accessed using the J20 header of the SAM-IoT board (unfortunately the UART pins are not broken out to either of the 2 headers, so refer to the [SAM-IoT board schematic](https://www.microchip.com/content/dam/mchp/documents/MCU32/ProductDocuments/UserGuides/SAM-IoT-WG-Development-Board-User-Guide.pdf) for connecting directly to the PB02 & PB03 pins).

<img src=".//media/image119.png"/>



### Sending Application Processor Telemetry Events

The `telemetry` command is executed using the CLI to update any of the available telemetry variables by using a pre-defined index to select the desired one

```bash
>telemetry <index>,<data>
```
NOTE: The number of characters for the CLI command cannot exceed a total of 80 characters

<img src=".//media/image120.png"/>



### Reading/Writing Application Processor Properties

The `property` command is executed using the CLI to update (write to) any of the available properties by using a pre-defined index to select the desired one

```bash
>property <index>,<data>
```
NOTE: The number of characters for the CLI command cannot exceed a total of 80 characters

<img src=".//media/image121.png"/>



### Dedicated Telemetry Interface (DTI)

The DTI is implemented on the SAM-IoT configured as a SPI Slave. The DTI should be used when larger telemetry chunks of data need to be sent which cannot be executed using the CLI (due to the limitation of the maximum number of characters that the CLI can process for a single command line).  The external application processor must be configured for SPI Master mode and be connected to SAM-IoT board's J-20 header using 3 wires (MOSI, SCK, CS).  The SPI Master must send a valid data frame in the correct format (header + payload) for every telemetry update operation.

For the most reliable performance, the SPI Master should adhere to the following operating parameters:

- Interframe delay = 500 msec minimum
- SCK = 30 kHz maximum
- [Recommendation] Issue the `telemetry 0,0` command on the CLI prior to sending an individual data frame (or right before the start of a group of consecutive data frames).  This command will ensure that the DTI receive buffer has been properly reset prior to receiving the first byte of the incoming data frame sent by the SPI Master

<img src=".//media/image122.png"/>

### Data Frame Format

Each SPI transaction must follow the format for sending out a valid data frame through the DTI.  A valid data frame must start with a 4-byte header followed by the payload data.  The command byte must be equal to the hex value corresponding to ASCII character 'T' or 't' (signifying that the data frame is to be processed as a `telemetry` command)

<img src=".//media/image123.png"/>

For example, the following data frame tells the SAM-IoT to update the `telemetry_Str_1` value in the Cloud with a 1KB array of bytes that begins with "START", terminated with "END", and every byte in between with consecutive '1's

<img src=".//media/image124.png"/>



## Program the DTI Test Platform using a PIC-IoT Development Board Configured as the SPI Master

1. Clone/download the SPI code example project by issuing the following commands in a `Command Prompt` or `PowerShell` window

   ```bash
   git clone https://github.com/randywu763/MCHP-IoT_SPI.git
   cd MCHP-IoT_SPI
   git submodule update --init
   ```

2. Connect the **PIC-IoT Development Board** to the PC, then make sure `CURIOSITY` device shows up as a disk drive on the `Desktop` or in a `File Explorer` window. Drag and drop (i.e. copy) the pre-built `*.hex` file (located in the folder at `MCHP-IoT_SPI` > `PIC-IoT_SPI-Master.X` > `dist` > `PIC24_IOT_WG` > `production`) to the `CURIOSITY` drive.

   <img src=".//media/image130.png"/>



## Installing the Development Tools

Embedded software development tools from Microchip need to be pre-installed in order to properly program the SAM-IoT WG Development Board and provision it for use with Microsoft Azure IoT services.

Click this link for the setup procedure: [Development Tools Installation](https://github.com/Azure-Samples/Microchip-SAM-IoT-Wx/blob/main/Dev_Tools_Install.md)



## Program SAM-IoT Development Board Configured as SPI Slave

1. Clone/download the MPLAB X demo project by issuing the following commands in a `Command Prompt` or `PowerShell` window.

   ```bash
   git clone https://github.com/Azure-Samples/Microchip-SAM-IoT-Wx.git
   cd Microchip-SAM-IoT-Wx
   git submodule update --init
   ```

2. Make the appropriate jumper connections between the J20 headers on both development boards.

   <img src=".//media/image125.png"/>

   

3. Connect the **SAM-IoT Development Board** to the PC, then make sure `CURIOSITY` device shows up as a disk drive on the `Desktop` or in a `File Explorer` window. 

4. Connect the board to PC, then make sure `CURIOSITY` device shows up as a disk drive on the `Desktop` or in a `File Explorer` window. Drag and drop (i.e. copy) the pre-built `*.hex` file (located in the folder at `Microchip-SAM-IoT-Wx` > `firmware` > `AzurePnPDps.X` > `dist` > `SAMD21_WG_IOT` > `production`) to the `CURIOSITY` drive

   [![img](.//media/image115.png)](https://github.com/Azure-Samples/Microchip-SAM-IoT-Wx/blob/main/media/image115.png)

5. Set up a Command Line Interface (CLI) to the board.

- Open a serial terminal (e.g. PuTTY, TeraTerm, etc.) and connect to the COM port corresponding to the **SAM-IoT Development Board** at `9600 baud` (e.g. open PuTTY Configuration window &gt; choose `session` &gt; choose `Serial`&gt; Enter the right COMx port). You can find the COM info by opening your PC’s `Device Manager` &gt; expand `Ports(COM & LPT)` &gt; take note of `Curiosity Virtual COM Port` .

<img src=".//media/image27.png">

4. Before typing anything in the terminal emulator window, **disable** the local echo feature in the terminal settings for best results.  In the terminal window, hit `[RETURN]` to bring up the Command Line Interface prompt (which is simply the `>` character). Type `help` and then hit `[RETURN]` to get the list of available commands for the CLI.  The Command Line Interface allows you to send simple ASCII-string commands to set or get the user-configurable operating parameters of the application while it is running.

<img src=".//media/image44.png" style="width:5.in;height:3.18982in"/>

5. In the terminal emulator window, set the debug messaging level to 0 to temporarily disable the output messages. The debug level can be set anywhere from 0 to 4.  Use the `debug <level>` command by manually typing it into the CLI.  The complete command must be followed by hitting `[RETURN]`

```bash
>debug 0
```

6. Perform a Wi-Fi scan to see the list of Access Points that are currently being detected by the board's Wi-Fi network controller.  Use  the `wifi` command's `scan` option by manually typing it into the CLI.  The complete command must be followed by hitting `[RETURN]`

```bash
>wifi -scan
```

7. Configure the SAM-IoT board's internal Wi-Fi settings with your wireless router’s SSID and password using the `wifi` command's `set` option by manually typing it into the CLI.  The complete command must be followed by hitting `[RETURN]` (there cannot be any spaces used in the SSID or password)

```bash
>wifi -set <NETWORK_SSID>,<PASSWORD>,<SECURITY_OPTION[1=Open|2=WPA|3=WEP]>
```

For example, if the SSID of the router is "MyWirelessRouter" and the WPA/WPA2 key is "MyRoutersPassword", the exact command to type into the CLI (followed by `[RETURN]`) would be

```bash
>wifi -set MyWirelessRouter,MyRoutersPassword,2
```

8. At the CLI prompt, type in the command `reset` and hit `[RETURN]` to restart the host application.  The Blue LED should eventually stay solidly ON to signify that the SAM-IoT board has successfully connected to the wireless router.

```bash
>reset
```

9. At this point, the board is connected to Wi-Fi, but has not yet established a connection with the cloud.  The `cloud` command can be used at any time to confirm the cloud connection status.  The complete command must be followed by hitting `[RETURN]`

```bash
>cloud -status
```

## Create an IoT Central Application for your Device

IoT Central allows you to create an application dashboard to monitor the telemetry and take appropriate actions based on customized rules.

1. Create a custom IoT Central application by starting with an existing [Microchip IoT Development Board Template](https://apps.azureiotcentral.com/build/new/fc996b98-956b-4b9e-936c-1bfe4b313cb4) (if there is a problem with loading the template, refer to the [Create an application](https://docs.microsoft.com/en-us/azure/iot-central/core/quick-deploy-iot-central) section to create your IoT Central application from scratch). If you are not currently logged into your [Microsoft account](https://account.microsoft.com/account), you will be prompted to sign in with your credentials to proceed. If you do not have an existing Microsoft account, go ahead and create one now by clicking on the `Create one!` link

2. Azure IoT Builder will guide you through the process of creating your application. Review and select the various settings for your IoT Central application (if needed, refer to [Create an application](https://docs.microsoft.com/en-us/azure/iot-central/core/quick-deploy-iot-central) for additional guidance on selecting the settings for your application). Do not click the `Create` button just yet - only after reviewing and taking into consideration the following recommendations:
  
    - Choose a unique `Application name` (which will result in a unique `URL`) for accessing your application. Azure IoT Builder will populate a suggested unique `Application name` which can/should be leveraged, resulting in a unique `URL` shown on the screen. Take note of the unique/customizable portion of the `URL` (e.g. "custom-1pfphmras2b" like shown in the below screen shot) as it will be needed in a future step (suggest copy/pasting the exact text into a text editor doc file as temporary storage for the name)

        <img src=".//media/image80a.png">

    - If you select the **Free** plan, you can connect up to 5 devices for free.  However, the free trial period will expire after 7 days which means a [paid pricing plan](https://azure.microsoft.com/en-us/pricing/details/iot-central/) will need to be selected to continue using the application.  Of course, there is nothing to stop you from creating a new free trial application but the device will need to be configured for the app from scratch.  Since the **Standard** plans each allow 2 free devices with no time-restricted trial period, if you only plan on evaluating 1 or 2 devices for connecting to the IoT Central app, then it's best to choose the **Standard 2** plan to get the highest total allowable number of messages (30K per month)

        <img src=".//media/image80b.png">

    - `Billing info` section: If there is an issue with selecting an existing subscription in the drop-down list (or no subscriptions appear in the list at all), click on the `Create subscription` link to create a new subscription to use for the creation of this application.  Take note of the exact subscription name (e.g. "Azure subscription 1" like shown in the below screen shot) which was selected as it will be needed in a future step (suggest copying/pasting the exact text into a text editor file as temporary storage for the name)
    
        <img src=".//media/image80c.png">

3. Click the `Create` button (the application will be automatically saved in your [IoT Central Portal](https://apps.azureiotcentral.com))

4. Using the individual enrollment method, register the device certificate with your custom IoT Central application by running the [pyazureutils](https://pypi.org/project/pyazureutils/) utility supplied by Microchip (which should already be installed) by executing the following steps:

    - Refer to the [Dev Tools Installation](./Dev_Tools_Install.md) procedure and confirm that `Azure CLI`, `Python`, and `pyazureutils` have all been previously installed. The device certificate should already exist/reside in the `ChainOfTrust` folder (which was generated by the [SAM-IoT WG Development Board (EV75S95A) Provisioning Tools Package for Windows](https://github.com/randywu763/sam-iot-provision))
    
    - Launch a `Command Prompt` or `PowerShell` window and then execute the command to navigate to the `ChainOfTrust` folder

        ```bash
        cd <MY_PATH>/sam-iot-provision-main/SAM_IoT_Certs_Generator/ChainOfTrust
        ```

    - Execute the following command line (filling in each of the parameters with all your specific options). You may be prompted to log into your Microsoft Azure account if you are not currently signed in after the `pyazureutils` command starts execution

        ```bash
        pyazureutils --subscription "<SUBSCRIPTION_NAME>" iotcentral register-device --certificate-file "<CERTIFICATE_NAME>" --template "<TEMPLATE_NAME>" --application-name "<APPLICATION_URL>"
        ```

        For example, based on the preceding example screenshots of building the application
    
        - <SUBSCRIPTION_NAME> = Azure Subscription 1
        - <CERTIFICATE_NAME> = device.crt
        - <TEMPLATE_NAME> = SAM_IoT_WM;2
        - <APPLICATION_URL> = custom-1pfphmras2b

        ```bash
        pyazureutils --subscription "Azure subscription 1" iotcentral register-device --certificate-file "device.crt" --template "SAM_IoT_WM;2" --application-name "custom-1pfphmras2b"
        ```

    - Upon successful completion of the `pyazureutils` operations, the final output messages should look something like the following:

        ```bash
        Registration:
        Using device template: SAM_IoT_WM;2 (dtmi:modelDefinition:dftia5bwj:un5msohpx)
        Creating device 'sn01239E946F011C66FE' from template 'dtmi:modelDefinition:dftia5bwj:un5msohpx'
        Checking device
        Creating device attestation using certificate
        Checking device attestation
        Registration complete!
        ```

    NOTE: If the `pyazureutils` command fails to register your device, an alternative method is to follow the procedure for [Creating an X.509 Enrollment Group](./IoT_Central_Enrollment_Group.md) to get your device connected using the group enrollment method

    **Future Consideration**: An enrollment group is an entry for a group of devices that share a common attestation mechanism. Using an enrollment group is recommended for a large number of devices that share an initial configuration, or for devices that go to the same tenant. Devices that use either symmetric key or X.509 certificates attestation are supported. [Create an X.509 Enrollment Group](./IoT_Central_Enrollment_Group.md) for your IoT Central application should the need ever arise in the future, when tens, hundreds, thousands, or even millions of devices (that are all derived from the same root certificate) need to connect to an application...

5. If not already active, launch a terminal emulator window and connect to the COM port corresponding to the SAM-IoT board at `9600` baud (**disable** local echo for the terminal settings for best results).  Hit `[RETURN]` to bring up the Command Line Interface prompt (which is simply the `>` character). Type `help` and then hit `[RETURN]` to get the list of available commands for the CLI.  The Command Line Interface allows you to send simple ASCII-string commands to set or get the user-configurable operating parameters of the application while it is running

    <img src=".//media/image44.png" style="width:5.in;height:3.18982in" alt="A screenshot of a cell phone Description automatically generated" />

6. Look up the `ID Scope` for the DPS created/used by your IoT Central application (navigate to your application's web page and using the left-hand navigation pane, select `Administration` > `Device connection`).  The `ID Scope` will be programmed/saved into the [ATECC608A](https://www.microchip.com/wwwproducts/en/atecc608a) secure element on the board in the next step

    <img src=".//media/image84.png" style="width:5.in;height:3.18982in" />

7. In the terminal emulator window, hit `[RETURN]` to bring up the Command Line Interface prompt (which is simply the `>` character>). At the CLI prompt, type in the `idscope <your_ID_scope>` command to set it (which gets saved in the [ATECC608A](https://www.microchip.com/wwwproducts/en/atecc608a) secure element on the board) and then hit `[RETURN]`.  The ID Scope can be read out from the board by issuing the `idscope` command without specifying any parameter on the command line

    <img src=".//media/image85.png" style="width:5.in;height:3.18982in" />

8. In the terminal emulator window, hit `[RETURN]` to bring up the CLI prompt. Type in the command `reset` and hit `[RETURN]`

9. Wait for the SAM-IoT board to connect to your IoT Central application’s DPS; the Blue and Green LEDs will be flashing and/or staying on at different times/rates (which could take up to a few minutes).  Eventually the Blue and Green LEDs should both remain constantly ON.

    NOTE: If the Red LED comes on, then something may have been incorrectly programmed (e.g. wrong firmware, ID scope was entered incorrectly, etc.)

10. At this point, the board should have established a valid cloud connection (this can be confirmed visually by the Green LED staying on constantly).  The `cloud` command can be used at any time to confirm the cloud connection status using the CLI.  The complete command must be followed by hitting `[RETURN]`

    ```bash
    >cloud -status
    ```

11. Go back to your web browser to access the Azure IoT Central application.  Use the left-hand side pane and select `Devices` > `All Devices`.  Confirm that your device is listed – the device name & ID is the Common Name of the device certificate (which should be `sn + {17-digit device ID}`)

    <img src=".//media/image86.png" style="width:5.in;height:1.38982in" alt="A screenshot of a cell phone Description automatically generated" />





## Validate Connection

Whenever the SW1 button is pressed on the PIC-IoT board, the red LED will toggle whenever a data frame has been sent out on the SPI bus.  Whenever a valid data frame has been received and properly decoded by the SAM-IoT, its yellow LED will toggle.

NOTE: It is strongly recommended to issue the `telemetry 0,0` command on the CLI prior to sending data frames out to the DTI.  It is not a requirement, but issuing this command will ensure that the DTI receive buffer has been properly reset to receive a data frame

<img src=".//media/image131.png"/>





## Data Visualization using the IoT Central Application

Using the dashboard drop-down selection list, choose the `SAM-IoT WM v2 Properties & Telemetry Data Buckets` dashboard in the IoT Central Template Application to see the last known received values for each of the 4 telemetry string values.

Note: you may need to configure each of the property tiles in the premade IoT Central Template application to the device you just connected and validated. To do this, click the "Edit" button at the top of your dashboard, then click "Configure" (gear icon) on each desired property tile. Then, select your device group (in this case, `SAM-IoT WM v2`) and your device as listed before (which should be `sn + {17-digit device ID}`).

![image132](.//media/image132.png)



Here are a list of suggested properties to include in the dashboard (see screenshot below for visual):

| **System Information**                                           | **App MCU Properties**                                           | **LED States**                                                   |                              |
| ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ | ---------------------------- |
| ATWINC1510 Firmware Version<br />Debug Level<br/>Disable Telemetry<br/>IP Address<br/>Set Telemetry Interval | App MCU Property 1<br/>App MCU Property 2<br/>App MCU Property 3<br/>App MCU Property 4 | Blue LED state<br/>Green LED state<br/>Red LED state<br/>Yellow LED state |                              |
| **Telemetry Data Bucket #1**                                 | **Telemetry Data Bucket #2**                                 | **Telemetry Data Bucket #3**                                 | **Telemetry Data Bucket #4** |
| App MCU Telemetry 1 (String)                                 | App MCU Telemetry 2 (String)                                 | App MCU Telemetry 3 (String)                                 | App MCU Telemetry 4 (String) |

<img src=".//media/image126.png"/>



### Confirmation of Reception for Every Data Frame Sent by the Application Processor

Observe each of the raw data messages received by the IoT Central application by clicking on the device and then selecting the `Raw Data` tab.  Note that each message contains a time stamp that can be used to verify the exact time each individual message was received by the IoT Central application.

<img src=".//media/image127.png"/>

<img src=".//media/image128.png"/>

<img src=".//media/image129.png"/>
