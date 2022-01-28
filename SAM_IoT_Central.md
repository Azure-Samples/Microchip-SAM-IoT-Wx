# Connecting the SAM-IoT WG Development Board (Part No. EV75S95A) to Azure IoT Central

## Introduction

[Azure IoT Central](https://docs.microsoft.com/en-us/azure/iot-central/core/overview-iot-central) is an IoT application platform that reduces the burden and cost of developing, managing, and maintaining enterprise-grade IoT solutions. Choosing to build with IoT Central gives you the opportunity to focus time, money, and energy on transforming your business with IoT data, rather than just maintaining and updating a complex and continually evolving IoT infrastructure.

The web UI lets you quickly connect devices, monitor device conditions, create rules, and manage millions of devices and their data throughout their life cycle. Furthermore, it enables you to act on device insights by extending IoT intelligence into line-of-business applications.

[IoT Plug and Play](https://docs.microsoft.com/en-us/azure/iot-develop/overview-iot-plug-and-play) enables solution builders to integrate IoT devices with their solutions without any manual configuration. At the core of IoT Plug and Play, is a device model that a device uses to advertise its capabilities to an IoT Plug and Play-enabled application. This model is structured as a set of elements that define:

- `Properties` that represent the read-only or writable state of a device or other entity. For example, a device serial number may be a read-only property and a target temperature on a thermostat may be a writable property

- `Telemetry` which is the data emitted by a device, whether the data is a regular stream of sensor readings, an occasional error, or an information message

- `Commands` that describe a function or operation that can be done on a device. For example, a command could reboot a gateway or take a picture using a remote camera

As a solution builder, you can use IoT Central to develop a cloud-hosted IoT solution that uses IoT Plug and Play devices. IoT Plug and Play devices connect directly to an IoT Central application where you can use customizable dashboards to monitor and control your devices. You can also use device templates in the IoT Central web UI to create and edit [Device Twins Definition Language (DTDL)](https://github.com/Azure/opendigitaltwins-dtdl) models.

## Program the Plug and Play Demo

1. Clone/download the MPLAB X demo project by issuing the following commands in a `Command Prompt` or `PowerShell` window

    ```bash
    git clone https://github.com/Azure-Samples/Microchip-SAM-IoT-Wx.git
    cd Microchip-SAM-IoT-Wx
    git submodule update --init
    ```

2. Connect the board to PC, then make sure `CURIOSITY` device shows up as a disk drive on the `Desktop` or in a `File Explorer` window. Drag and drop (i.e. copy) the pre-built `*.hex` file (located in the folder at `Microchip-SAM-IoT-Wx` > `firmware` > `AzurePnPDps.X` > `dist` > `SAMD21_WG_IOT` > `production`) to the `CURIOSITY` drive 

    <img src=".//media/image115.png">

    NOTE: If this file copy operation fails for any reason, [Make and Program Device](./AzurePnP_MPLABX.md) by building the MPLAB X source code project that was used to generate the `*.hex` file

3. Set up a Command Line Interface (CLI) to the board

    - Open a serial terminal (e.g. PuTTY, TeraTerm, etc.) and connect to the COM port corresponding to your board at `9600 baud` (e.g. open PuTTY Configuration window &gt; choose `session` &gt; choose `Serial`&gt; Enter the right COMx port). You can find the COM info by opening your PC’s `Device Manager` &gt; expand `Ports(COM & LPT)` &gt; take note of `Curiosity Virtual COM Port` 

        <img src=".//media/image27.png">

 4. Before typing anything in the terminal emulator window, **disable** the local echo feature in the terminal settings for best results.  In the terminal window, hit `[RETURN]` to bring up the Command Line Interface prompt (which is simply the `>` character). Type `help` and then hit `[RETURN]` to get the list of available commands for the CLI.  The Command Line Interface allows you to send simple ASCII-string commands to set or get the user-configurable operating parameters of the application while it is running

    <img src=".//media/image44.png" style="width:5.in;height:3.18982in" alt="A screenshot of a cell phone Description automatically generated" />

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

        NOTE: If the message `Something went wrong` appears underneath the `Azure subscription` field, open up a web browser and log into your account using the [Azure portal](https://portal.azure.com) then retry selecting (or creating) a valid subscription

        <img src=".//media/image80d.png">
        
3. Click the `Create` button (the application will be automatically saved in your [IoT Central Portal](https://apps.azureiotcentral.com))

4. Using the individual enrollment method, register the device certificate with your custom IoT Central application by running the [pyazureutils](https://pypi.org/project/pyazureutils/) utility supplied by Microchip (which should already be installed) by executing the following steps:

    - Refer to the [Dev Tools Installation](./Dev_Tools_Install.md) procedure and confirm that `Azure CLI`, `Python`, and `pyazureutils` have all been previously installed. The device certificate should already exist/reside in the `ChainOfTrust` folder (which was generated by the [SAM-IoT WG Development Board (EV75S95A) Provisioning Tools Package for Windows](https://github.com/randywu763/sam-iot-provision))
    
    - Launch a `Command Prompt` or `PowerShell` window and then execute the command to navigate to the `ChainOfTrust` folder

        ```bash
        cd <MY_PATH>/sam-iot-provision-main/SAM_IoT_Certs_Generator/ChainOfTrust
        ```

    - Execute the following command line (filling in each of the parameters with all your specific options). You may be prompted to log into your Microsoft Azure account if you are not currently signed in after the `pyazureutils` command starts execution

        ```bash
        pyazureutils --subscription "<SUBSCRIPTION_NAME>" iotcentral register-device --certificate-file "<CERTIFICATE_NAME>" --template "<TEMPLATE_NAME>" --application-name "<CUSTOMIZED_PORTION_OF_URL>"
        ```

        For example, based on the preceding example screenshots of building the application
    
        - <SUBSCRIPTION_NAME> = Azure Subscription 1
        - <CERTIFICATE_NAME> = device.crt
        - <TEMPLATE_NAME> = SAM_IoT_WM;2
        - <CUSTOMIZED_PORTION_OF_URL> = custom-1pfphmras2b

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

5. Launch a terminal emulator window and connect to the COM port corresponding to the SAM-IoT board at `9600` baud (**disable** local echo for the terminal settings for best results).  Hit `[RETURN]` to bring up the Command Line Interface prompt (which is simply the `>` character). Type `help` and then hit `[RETURN]` to get the list of available commands for the CLI.  The Command Line Interface allows you to send simple ASCII-string commands to set or get the user-configurable operating parameters of the application while it is running

    <img src=".//media/image44.png" style="width:5.in;height:3.18982in" alt="A screenshot of a cell phone Description automatically generated" />

6.	Look up the `ID Scope` for the DPS created/used by your IoT Central application (navigate to your application's web page and using the left-hand navigation pane, select `Administration` > `Device connection`).  The `ID Scope` will be programmed/saved into the [ATECC608A](https://www.microchip.com/wwwproducts/en/atecc608a) secure element on the board in the next step

    <img src=".//media/image84.png" style="width:5.in;height:3.18982in" alt="A screenshot of a cell phone Description automatically generated" />

7. In the terminal emulator window, hit `[RETURN]` to bring up the Command Line Interface prompt (which is simply the `>` character>). At the CLI prompt, type in the `idscope <your_ID_scope>` command to set it (which gets saved in the [ATECC608A](https://www.microchip.com/wwwproducts/en/atecc608a) secure element on the board) and then hit `[RETURN]`.  The ID Scope can be read out from the board by issuing the `idscope` command without specifying any parameter on the command line

    <img src=".//media/image85.png" style="width:5.in;height:3.18982in" alt="A screenshot of a cell phone Description automatically generated" />

8. In the terminal emulator window, hit `[RETURN]` to bring up the CLI prompt. Type in the command `reset` and hit `[RETURN]`

9. Wait for the SAM-IoT board to connect to your IoT Central application’s DPS; the Blue and Green LEDs will be flashing and/or staying on at different times/rates (which could take up to a few minutes).  Eventually the Blue and Green LEDs should both remain constantly ON.

    NOTE: If the Red LED comes on, then something may have been incorrectly programmed (e.g. wrong firmware, ID scope was entered incorrectly, etc.)

10. At this point, the board should have established a valid cloud connection (this can be confirmed visually by the Green LED staying on constantly).  The `cloud` command can be used at any time to confirm the cloud connection status using the CLI.  The complete command must be followed by hitting `[RETURN]`
    ```bash
    >cloud -status
    ```

11. Go back to your web browser to access the Azure IoT Central application.  Use the left-hand side pane and select `Devices` > `All Devices`.  Confirm that your device is listed – the device name & ID is the Common Name of the device certificate (which should be `sn + {17-digit device ID}`)

    <img src=".//media/image86.png" style="width:5.in;height:1.38982in" alt="A screenshot of a cell phone Description automatically generated" />

12. If desired, change the Device name by clicking on `Manage device` > `Rename`

    <img src=".//media/image87.png" style="width:5.in;height:1.18982in" alt="A screenshot of a cell phone Description automatically generated" />

13. Click on the `Command` tab; type `PT5S` in the `Reboot delay` field and then click on `Run` to send the command to the device to reboot in 5 seconds

    <img src=".//media/image88.png" style="width:5.in;height:1.58982in" alt="A screenshot of a cell phone Description automatically generated" />

14. Within 5 seconds of sending the Reboot command, the SAM-IoT development board should reset itself.  Once the Blue and Green LED's stay constantly ON, press the SW0 and SW1 buttons (the Red LED may toggle with each button press)

    <img src=".//media/image89.png" style="width:5.in;height:1.28982in" alt="A screenshot of a cell phone Description automatically generated" />

15. Click on the `Raw data` tab and confirm that the button press telemetry messages were received

    <img src=".//media/image90.png" style="width:5.in;height:1.98982in" alt="A screenshot of a cell phone Description automatically generated" />

16. Click on the `Refresh` icon to display all messages received since the previous page refresh operation.  Confirm that periodic telemetry messages are being continuously received approximately every 10 seconds (the default interval value for the `telemetryInterval` property)

    <img src=".//media/image91.png" style="width:5.in;height:1.58982in" alt="A screenshot of a cell phone Description automatically generated" />

    <img src=".//media/image92.png" style="width:5.in;height:2.12982in" alt="A screenshot of a cell phone Description automatically generated" />

17. Increase the ambient light source shining on top of the board. Wait approximately 30 seconds.  Click on the `Refresh` icon to confirm that the light sensor value has increased

    <img src=".//media/image93.png" style="width:5.in;height:2.18982in" alt="A screenshot of a cell phone Description automatically generated" />

## Connect your Device to the Dashboard for Data Visualization

1. Navigate to the left-hand vertical toolbar and click on the `Dashboards` icon

    <img src=".//media/image100.png" style="width:5.in;height:0.98982in" alt="A screenshot of a cell phone Description automatically generated" />

2. Towards the top of the web page, click on the `Edit` icon

    <img src=".//media/image101.png" style="width:5.in;height:0.38982in" alt="A screenshot of a cell phone Description automatically generated" />

3. For **all** of the existing tiles named `Light` or `Temperature`, click on the upper right-hand corner of the tile to select `Configure`

    <img src=".//media/image102a.png" style="width:5.in;height:2.18982in" alt="A screenshot of a cell phone Description automatically generated" />
    <img src=".//media/image102b.png" style="width:5.in;height:2.18982in" alt="A screenshot of a cell phone Description automatically generated" />

4. Select `Device Group` > `SAM-IoT WM - All devices` and then check the box for your specific device name for `Devices`

    <img src=".//media/image103.png" style="width:5.in;height:2.08982in" alt="A screenshot of a cell phone Description automatically generated" />

5. Under the `Telemetry` category, click on `+ Capability` and select the parameter pertaining to the title of the tile (e.g. `Brightness from light sensor` for each of the `Light` tiles or `Temperature` for each of the `Temperature` tiles)

    <img src=".//media/image104a.png" style="width:5.in;height:0.89082in" alt="A screenshot of a cell phone Description automatically generated" />
    <img src=".//media/image104b.png" style="width:5.in;height:2.18982in" alt="A screenshot of a cell phone Description automatically generated" />
    <img src=".//media/image104c.png" style="width:5.in;height:1.18982in" alt="A screenshot of a cell phone Description automatically generated" />

6. Click on `Update` and repeat the process for the remainder of the existing tiles

    <img src=".//media/image105.png" style="width:5.in;height:0.48982in" alt="A screenshot of a cell phone Description automatically generated" />

7. After every tile has been configured to visualize your device's sensor data, click on the `Save` icon to save the latest changes to the dashboard

    <img src=".//media/image106.png" style="width:5.in;height:0.38982in" alt="A screenshot of a cell phone Description automatically generated" />

8. Confirm that the dashboard is being continuously updated with the expected telemetry data received from the device.  For example, adjust the ambient light source directed at the board and observe that the light sensor values are changing accordingly

    <img src=".//media/image107.png" style="width:5.in;height:2.58982in" alt="A screenshot of a cell phone Description automatically generated" />

8. To access your IoT Central application(s) in the future, go to [Azure IoT Central](https://apps.azureiotcentral.com) and click on `My apps`

    <img src=".//media/image108.png" style="width:5.in;height:1.98982in" alt="A screenshot of a cell phone Description automatically generated" />

## Expand the Dashboard with Additional Tiles

To create additional tiles for your IoT Central dashboard, refer to [Configure the IoT Central application dashboard](https://docs.microsoft.com/en-us/azure/iot-central/core/howto-add-tiles-to-your-dashboard). The below screen captures show additional possibilities of dashboard components that can highlight the telemetry data and properties facilitated by the `Plug and Play` interface.  Note that multiple devices can be selected for each tile to allow groups of devices to be visualized within a single tile. 

<img src=".//media/image95.png" style="width:5.in;height:3.34982in" alt="A screenshot of a cell phone Description automatically generated" />

<img src=".//media/image96.png" style="width:5.in;height:4.4182in" alt="A screenshot of a cell phone Description automatically generated" />

<img src=".//media/image97.png" style="width:5.in;height:3.34982in" alt="A screenshot of a cell phone Description automatically generated" />

<img src=".//media/image98.png" style="width:5.in;height:3.34982in" alt="A screenshot of a cell phone Description automatically generated" />
