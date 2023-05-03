## Importing Materials Errors


##### Material expression called Compiler->TextureParameter() without implementing UMaterialExpression::GetReferencedTexture properly
```
Assertion failed: TextureReferenceIndex != INDEX_NONE [File:D:\build\++UE5\Sync\Engine\Source\Runtime\Engine\Private\Materials\HLSLMaterialTranslator.cpp] [Line: 6794] 
Material expression called Compiler->TextureParameter() without implementing UMaterialExpression::GetReferencedTexture properly
UnrealEditor.exe has triggered a breakpoint.
```

> JsonAsAsset may encounter issues when importing a JSON file that contains material data (mainly UMaterial) which triggers the error message "Material expression called Compiler->TextureParameter() without implementing UMaterialExpression::GetReferencedTexture properly".

To resolve this issue, *we recommend that you first import the material functions that are associated with the material*. This may allow the plugin to properly reference the textures and resolve the error. We also advise that you import the material only after the material functions have been successfully imported, as this may help to prevent the error from occurring.

Please note that this is a known issue with JsonAsAsset and we are trying to find out a way to fix it. We apologize for any inconvenience this may cause and appreciate your patience and understanding as we work to improve the plugin's functionality.
