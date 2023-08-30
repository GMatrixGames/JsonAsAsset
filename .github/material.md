## Importing Materials Errors


##### Material expression called Compiler->TextureParameter() without implementing UMaterialExpression::GetReferencedTexture properly
```
Assertion failed: TextureReferenceIndex != INDEX_NONE [File:D:\build\++UE5\Sync\Engine\Source\Runtime\Engine\Private\Materials\HLSLMaterialTranslator.cpp] [Line: 6794] 
Material expression called Compiler->TextureParameter() without implementing UMaterialExpression::GetReferencedTexture properly
UnrealEditor.exe has triggered a breakpoint.
```

> JsonAsAsset may encounter issues when importing a JSON file that contains material data (mainly UMaterial) which triggers the error message "Material expression called Compiler->TextureParameter() without implementing UMaterialExpression::GetReferencedTexture properly".

To resolve this issue, *we recommend that you first try enabling package saving*. This may allow Unreal Engine to properly reference the textures and resolve the error. (a crash may occur, restart and everything should be alright)
