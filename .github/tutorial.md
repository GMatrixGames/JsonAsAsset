[*JsonAsAsset*](https://github.com/Tectors/JsonAsAsset) is a powerful Unreal Engine plugin developed mainly developed by [Tector](https://twitter.com/tectow) and [GMatrix](https://twitter.com/GMatrixGames). Allowing modders to easily import JSON data from FModel into their Unreal Engine projects and convert it into assets that can be used in their games, or used for modding.

<a name="table-of-contents"></a>
## Table of Contents

> 1. [Intro to the JsonAsAsset Plugin](#intro)
> 2. [Installing JsonAsAsset](#installation)
> 2. [Using JsonAsAsset](#using_jaa)

<a name="intro"></a>
## 1. Intro to the JsonAsAsset Plugin

> The plugin features a user-friendly interface that makes it easy for developers to import **JSON data** from *FModel* and map it to appropriate data structures within Unreal Engine to create assets based on them. This includes the ability to create curves, materials, animation curve data, and other assets based on the imported data. 

Overall, JsonAsAsset is a powerful plugin that can help game developers save time and improve their productivity when working with FModel data. With its advanced features, user-friendly interface, and seamless integration with Unreal Engine, it is a must-have tool for anyone working on Fortnite-related projects or other games that require the use of FModel data.

In this tutorial, we will explore the powerful JsonAsAsset plugin for Unreal Engine, which allows modders developers to easily convert JSON data obtained from FModel into Unreal Engine Assets. FModel is a popular tool used to extract data from various game files, and with the help of JsonAsAsset, modders developers can seamlessly import this data into Unreal Engine, saving time and effort.

Throughout this tutorial, we will guide you through the process of using the JsonAsAsset plugin to import JSON data from FModel into Unreal Engine. We will cover topics such as configuring the plugin, importing data, and importing it into [Unreal Editor For Fortnite](https://store.epicgames.com/en-US/p/fortnite--uefn). By the end of this tutorial, you will have a solid understanding of how to use JsonAsAsset to streamline your workflow and easily import JSON data from FModel into Unreal Engine.

<a name="installation"></a>
## 2. Installation
> JsonAsAsset may not work for every Unreal Engine 5 version, please check Releases to see compatibility. Unreal Engine 4 is not supported at the moment, and is not planned to be supported.

1. Go to the [Releases page](https://github.com/Tectors/JsonAsAsset/releases) for the plugin.
2. Download the release that matches your version of Unreal Engine. If there isn't a release that matches your version, you will need to compile the plugin yourself.
3. Extract the downloaded files to your project's Plugins folder. If there isn't a Plugins folder, create one in the root directory of your project.
4. Open your Unreal Engine project.
5. Click on Edit -> Plugins.
6. In the Plugins window, search for "JsonAsAsset" and enable it.
7. Restart the editor for the changes to take effect.

Additionally, to make sure the plugin works properly, you have to click on the JSON logo on the toolbar and press Open Plugin Settings. From there, you can double-click on the file selector for the property Export Directory and navigate to the directory where your FModel exports JSON from.

<a name="using_jaa"></a>
## 3. Importing JSON files
> Please be aware that the use of this plugin should be for personal and educational purposes only. Do not use it to create and distribute commercial products without proper licensing and permissions. It is important to respect the intellectual property of others and only use assets that you have the right to use.

1. Click on the [**JSON logo**](https://www.json.org) on your toolbar.
2. Select *"JsonAsAsset"* from the menu of buttons.
3. In the file browser that appears, select the JSON file you want to import.
4. Click "Open".
5. You can now use the imported asset in your Unreal Engine project.

### Importing Materials from Fortnite

JsonAsAsset can also be used to import materials from Fortnite into Unreal Engine, and then into the Unreal Editor to add materials that aren't normally accessible. To do this, follow these steps:

1. Open FModel to export the material as a JSON file.
2. Import the JSON file into Unreal Engine using JsonAsAsset.
3. Open the Unreal Editor for Fortnite to your project.
4. In the Content Browser of Unreal Engine, navigate to the imported material asset.
5. Right-click on the asset and select "Asset Actions -> Migrate"
6. Select the destination folder in your project.
7. Click "Migrate."
