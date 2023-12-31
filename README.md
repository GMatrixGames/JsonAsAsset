# JsonAsAsset

<img align="right" width="180" height="180" src="https://raw.githubusercontent.com/Tectors/JsonAsAsset/main/JsonAsAsset/Resources/JAA_Logo.png?raw=true">

[![Discord](https://img.shields.io/badge/Join%20Discord-Collector?color=7289DA&label=JsonAsAsset&logo=discord&logoColor=7289DA&style=for-the-badge)](https://discord.gg/h9s6qpBnUT)
![GitHub Repo stars](https://img.shields.io/github/stars/Tectors/JsonAsAsset?style=for-the-badge&logo=&color=lightgrey)
![Repo size](https://img.shields.io/github/repo-size/Tectors/JsonAsAsset?label=total%20size&style=for-the-badge&logo=&color=lightgrey&logoColor=lightgrey)
![Maintenance](https://img.shields.io/maintenance/yes/2024?style=for-the-badge&logo=&color=lightgrey)

Unreal Engine plugin to allow conversion between [JSON](https://www.json.org/json-en.html) to [Unreal Engine](https://www.unrealengine.com/en-US).

**Editor Only Data**:
<br> In Unreal Engine 5.2 and above, editor only data has been introduced to Unreal Engine. Allowing us to import materials and material functions into our own project from the pak files of a game.

**Contributors**:
<br> [Tector](https://github.com/Tectors), [TajGames](https://github.com/), [GMatrix](https://github.com/GMatrixGames), and [Zylox](https://github.com/0xZylox), and massive thanks to the people who contributed to [UEAssetToolkit](https://github.com/Buckminsterfullerene02/UEAssetToolkit-Fixes)!

**Artwork**:

- *Thumbnail*: [*JSON.org*](https://www.json.org/json-en.html)
- *JsonAsAsset Logo*: *[@Tevtongermany](https://github.com/Tevtongermany)*

-----------------

**Table of Contents**:
<img align="right" width="55%" height="100%" src="https://github.com/Tectors/JsonAsAsset/blob/main/JsonAsAsset/Resources/Icon_FullScreen.png">
<br> 

> 1. [Introduction to JsonAsAsset](#intro)
> 1. [Installing JsonAsAsset](#install)  
>    2.1 [Setting Up JAA Settings](#setup-jaa)  
>    2.2 [Export Directory](#export-directory)  
> 3. [Local Fetch](#setup-local-fetch) 

<a name="intro"></a>
## 1. Introduction to JsonAsAsset
> Please be aware that the use of this plugin should be for personal and educational purposes only. Do not use it to create and distribute commercial products without proper licensing and permissions. It is important to respect the intellectual property of others and only use assets that you have the right to use.

JsonAsAsset (or sometimes refered as *`JAA`*) is a plugin to create assets from json data obtained from [FModel](https://fmodel.app) *(Software for exploring Unreal Engine games)*. We are in no way responsible for what may be done with the created content, but you are free to use this tool as much as you like. Here are the supported asset types in JsonAsAsset:

###### Materials
 - Material
 - MaterialFunction
 - MaterialParameterCollection
 - PhysicalMaterial
 - SubsurfaceProfile
     
###### Curve Asset Types
 - CurveFloat
 - CurveTable
 - CurveVector
 - CurveLinearColor
 - CurveLinearColorAtlas

###### Skeleton/Animation Asset Types
 - SkeletalMeshLODSettings
 - Animation (curves, sync markers)

###### Sound Asset Types
 - SoundAttenuation
 - SoundConcurrency
 - ReverbEffect

###### Data Asset Types
- DataAsset
- DataTable

The plugin features a user-friendly interface that makes it easy for developers to import JSON data from FModel and map it to appropriate data structures within Unreal Engine to create assets based on them. This includes the ability to create curves, materials, animation curve data, and other assets based on the imported data. However, please know that this plugin is not 100% perfect and errors happen sometimes!

In this short documentation, we will learn how to use this powerful Unreal Engine plugin. And we hope you enjoy it!

<a name="install"></a>
## 2. Installing JsonAsAsset
> JsonAsAsset may not work for every Unreal Engine 5 version, please check Releases to see compatibility. Unreal Engine 4 is not supported at the moment, and is not planned to be supported.

1. Go to the [Releases page](https://github.com/Tectors/JsonAsAsset/releases) for the plugin.
2. Download the release that matches your version of Unreal Engine. If there isn't a release that matches your version, you will need to compile the plugin yourself.
3. Extract the downloaded files to your project's Plugins folder. If there isn't a Plugins folder, create one in the root directory of your project.
4. Open your Unreal Engine project.
5. Click on Edit -> Plugins.
6. In the Plugins window, search for "JsonAsAsset" and enable it.
7. Restart the editor for the changes to take effect.

<a name="setup-jaa"></a>
#### 2.1 Setting Up JAA Settings
> JsonAsAsset needs settings to be modified accordingly to properly handle assets. For example, the plugin has to seperate your export folder directory from the actual path to the game. 

If you haven't already, install FModel and setup it up correctly.
<img align="left" width="150" height="150" src="https://github.com/Tectors/JsonAsAsset/blob/main/JsonAsAsset/Resources/ButtonIcon_FModel.png?raw=true">
The JSON format/file has to be from a program that fits the format of FModel's JSON export files:
<br><br>

- [FModel](https://fmodel.app) *(Software for exploring Unreal Engine games)*

-------------------

Now that you've installed FModel and setup it up correctly, we can continue to setting up JsonAsAsset for our own Unreal Engine project. Also, FModel's data is the meat and bones of the plugin, without the data being correctly the same format, the plugin won't work.

<a name="export-directory"></a>
##### 2.2 Setting up Export Directory
<img align="right" width="300" height="180" src=https://github.com/Tectors/JsonAsAsset/assets/73559984/aad4e86a-6f0b-4e66-aef1-13d30d8215de)>
Upon launching your unreal engine project, you should of seen a notification asking you to change your export directory in the plugin settings, if you've already done that, skip this part.

<br>Without changing your export directory, it will cause the plugin to crash your entire project and will make you restart. We need to change the export directory to allow the plugin to differentiate between what's your directory, and what's the game path it should put it in.

First, open up to the JsonAsAsset plugin settings (basically do what's on the picture on the right) and make sure you are looking at the property "Export Directory".

Now open up FModel, and go to your settings. `(Settings -> General)` There will be a setting called "Output Directory", copy that and go back to Unreal Engine. Now you need to click on the three dots and jump to the folder you copied and go to the folder named "Exports", then press "Select Folder".

> We also recommend you enable *"Allow Package Saving"* for less issues during importing. 

-------------------

That is the basic setup done! However, if you want to import materials or linear curve atlas at bulk (ex: importing all material functions and their textures), then you're gonna need to setup `Local Fetch`.

<a name="setup-local-fetch"></a>
## 3. Setting Up *Local Fetch*
> Local Fetch is a local API to assist JsonAsAsset by acting as a "FModel", and it supplies textures and asset data to import at runtime. Especially if you're wanting to import materials. It uses CUE4Parse just like FModel.
<img align="right" width="461.5" height="164" src=https://github.com/Tectors/JsonAsAsset/assets/73559984/cddf0ea7-2499-4b39-a7af-e6f27ec5148e>

#### **NOTE:** Please make sure you have the plugin in your project's directory and not in the Engine.
> The reason being is that the Local Fetch API won't be able to find your config file (settings). 

Before we can launch up Local Fetch and get started on automated references. You need to put in all the information about your game first.

This is required as CUE4Parse needs this information to setup file providers and start reading the game files. A lot of these settings are the same as FModel, however make sure you always manually selecting a file/directory using UE's file selecter.

###### Launching Local Fetch
<img align="right" width="461.5" height="250" src=https://github.com/Tectors/JsonAsAsset/assets/73559984/4688482d-0854-4a62-83cf-fc055d657284>

> You must launch Local Fetch through UE, and not by the executable file. The reason being is that the local host port is different when you launch it through UE, so it's important you do so.

Once you've fully setup the settings for Local Fetch, you can now launch the api without any issues hopefully.

-------------------

Go ahead and click on the JsonAsAsset logo (<img width="25" height="25" src=https://github.com/Tectors/JsonAsAsset/assets/73559984/b90ab71f-d9ac-4349-96eb-620aadf7812f>) and hover over the list `"Command-line Application"` and press `"Execute JsonAsAsset API (.EXE)"`.

A window should pop-up, and once the console says `[CORE] Running API`, Local Fetch has been successfully started!
