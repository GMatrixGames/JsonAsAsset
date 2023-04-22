# JsonAsAsset

![GitHub Repo stars](https://img.shields.io/github/stars/Tectors/JsonAsAsset?style=for-the-badge&logo=&color=lightgrey)
![Repo size](https://img.shields.io/github/repo-size/Tectors/JsonAsAsset?label=total%20size&style=for-the-badge&logo=&color=lightgrey&logoColor=lightgrey)
![Maintenance](https://img.shields.io/maintenance/yes/2023?style=for-the-badge&logo=&color=lightgrey)

Unreal Engine plugin to allow conversion between [JSON](https://www.json.org/json-en.html) to [Unreal Engine](https://www.unrealengine.com/en-US). Supporting Unreal Engine 5.

**Editor Only Data**:
<br> In Unreal Engine 5.2 and above, editor only data has been introduced to Unreal Engine. Allowing us to import materials and material functions into our own project from the pak files of a game.

**Contributors**:
<br> [Tector](https://github.com/Tectors), [TajGames](https://github.com/), and [GMatrix](https://github.com/GMatrixGames)

## What's the file format?
<img align="right" width="150" height="150" src="https://raw.githubusercontent.com/4sval/FModel/master/FModel/FModel.ico">
The JSON format/file has to be from a program that fits the format of FModel's JSON export files:
<br><br>

- [FModel](https://fmodel.app) *(Software for exploring Unreal Engine games)*

However the version of your FModel application may affect the plugin's processing and parsing. For example, some FModel versions have different object names for curve data.

## How is this usable?
<img align="right" width="350" height="200" src="https://user-images.githubusercontent.com/73559984/232369173-74df3203-3af8-48e3-8d54-c28add197832.gif">

**Importing JsonAsAsset**:
<br> To import JsonAsAsset into your project, you simply head over to the [Releases](https://github.com/Tectors/JsonAsAsset/releases) and get a release that matches you unreal engine version. If not, compile the plugin yourself.

**Using JsonAsAsset**:
<br> After importing the plugin, click on the JSON logo on your toolbar and press JsonAsAsset (at the bottom) to select a JSON file. Then, it should import the asset and you can use it.

   -----------

### What's supported?
*Asset types are still being added to this plugin, contribute if you like*

- Material
- Material Function
- Material Instance Constant
- Material Parameter Collection
- CurveFloat
- CurveLinearColor
- CurveLinearColorAtlas
- SubsurfaceProfile
- SkeletalMeshLODSettings
- Animation (curves, sync markers)
- SoundAttenuation
- ReverbEffect

### JSON Logo
The logo used in this plugin is from: [JSON.org](https://www.json.org/json-en.html)
