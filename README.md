# JsonAsAsset
<img align="right" width="168" height="90" src="https://user-images.githubusercontent.com/73559984/224509260-48a78275-f5dd-4a20-b0f4-399fb6913d98.png">

   **Editor Only Data**:
 <br> In Unreal Engine 5.2 and above, editor only data has been introduced to Unreal Engine. Allowing us to import materials and material functions into our own project from the pak files of a game.

Unreal Engine plugin to allow conversion between [JSON](https://www.json.org/json-en.html) to Unreal Engine. Supporting Unreal Engine 5. 

> A editor plugin to allow JSON files from [FModel](https://fmodel.app) to a asset in the Content Browser

## What's the file format?
<img align="right" width="150" height="150" src="https://raw.githubusercontent.com/4sval/FModel/master/FModel/FModel.ico">
The JSON format/file has to be from a program that fits the format of FModel's JSON export files:

- [FModel](https://fmodel.app)
> Software for exploring Unreal Engine games

However the version of your FModel application may affect the plugin's processing and parsing. For example, some FModel versions have different object names for curve data.

## How's this used?
<img align="right" width="350" height="200" src="https://user-images.githubusercontent.com/73559984/224574869-6365f76f-8684-4788-b3c2-ad9acf832984.gif">
      You can use this plugin for many reasons, but I personally use it for a project to recreate a game.
  <br> <br> 

  **Importing JsonAsAsset**:
 <br> To import JsonAsAsset into your project, you simply head over to the [Releases](https://github.com/Tectors/JsonAsAsset/releases) and get a release that matches you unreal engine version. If not, compile the plugin yourself.
 
   **Using JsonAsAsset**:
 <br> After importing the plugin, click on the window tab (top left) and press JsonAsAsset (at the bottom) to select a JSON file. Then, it should import the asset and you can use it.

_Physic assets aren't supported due to the lack of information we have on porting them into Unreal Engine, if you have knowledge on how to support Physic assets let me know or contribute to this project._

## What's supported?
- CurveFloat
- CurveLinearColor
- SkeletalMeshLODSettings
- SubsurfaceProfile
- Animation (curves, sync markers)
- ReverbEffect
- SoundAttenuation
- MaterialParameterCollection

## Contributors
- [Tector](https://github.com/Tectors)
- [TajGames](https://github.com/)
- [GMatrix](https://github.com/GMatrixGames)
