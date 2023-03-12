# JsonAsAsset
<img align="right" width="168" height="90" src="https://user-images.githubusercontent.com/73559984/224509260-48a78275-f5dd-4a20-b0f4-399fb6913d98.png">

Unreal Engine plugin to allow conversion between [JSON](https://www.json.org/json-en.html) to Unreal Engine. Supports Unreal Engine 4 and Unreal Engine 5. 

> A editor plugin to allow JSON files from [FModel](https://fmodel.app) to a asset in the Content Browser

## What's the file format?
<img align="right" width="150" height="150" src="https://raw.githubusercontent.com/4sval/FModel/master/FModel/FModel.ico">
The JSON format/file has to be from a program that fits the format of FModel's JSON export files:

- [FModel](https://fmodel.app)
> Software for exploring Unreal Engine games

## What's supported?
- CurveFloat
- CurveLinearColor
- SkeletalMeshLODSettings
- Skeleton (sockets, virtual bones, slot groups, blend profiles)
- SubsurfaceProfile
- ~~PhysicsAsset~~
- Animation (curves, sync markers)
- ReverbEffect
- SoundAttenuation

## At the moment, Unreal Engine 5 is primarily being used to test. Unreal Engine 4 will supported later 

_Physic assets aren't supported due to the lack of information we have on porting them into Unreal Engine, if you have knowledge on how to support Physic assets let me know or contribute to this project._

## Contributors
- [Tector](https://github.com/Tectors)
- [Tajgames](https://github.com/)
- [GMatrix](https://github.com/GMatrixGames)
