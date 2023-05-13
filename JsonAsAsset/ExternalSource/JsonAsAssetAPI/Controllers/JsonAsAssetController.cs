using Microsoft.AspNetCore.Mvc;
using JsonAsAssetApi.Data;
using CUE4Parse.FileProvider;
using CUE4Parse.UE4.Assets.Exports;
using Newtonsoft.Json;
using CUE4Parse.Utils;
using CUE4Parse.UE4.Assets.Exports.Texture;
using CUE4Parse_Conversion.Textures;
using SkiaSharp;

namespace JsonAsAssetAPI.Controllers
{
    [Route("api/[controller]")]
    [ApiController]
    public class JsonAsAssetController : ControllerBase
    {
        private readonly ApiContext _context;
        private DefaultFileProvider Provider;

        public JsonAsAssetController(ApiContext context)
        {
            _context = context;
            Provider = Globals.Provider;
        }

        [HttpGet("/api/v1/export")]
        public ActionResult Get(bool raw, string path)
        {
            // Try to load object, if failed, return message
            try
            {
                switch(raw)
                {
                    // Raw exports
                    case true:
                        // Add .uasset to path
                        if (!path.EndsWith(".uasset") && !path.Contains("."))
                            path = path + ".uasset";

                        // Credit to MoutainFlash:
                        //  - https://gist.github.com/MinshuG/55f0da93fb839d41050e634b288e81b1
                        //    : (merging editor only data)
                        var objectPath = path.SubstringBeforeLast('.') + ".o." + path.SubstringAfterLast('.');
                        var exports = Provider.LoadAllObjects(path);
                        var finalExports = new List<UObject>();
                        finalExports.AddRange(exports);

                        var mergedExports = new List<UObject>();
                        if (Provider.TryLoadPackage(objectPath, out var editorAsset))
                        {
                            foreach (var export in exports)
                            {
                                var editorData = editorAsset.GetExportOrNull(export.Name + "EditorOnlyData");
                                if (editorData != null)
                                {
                                    export.Properties.AddRange(editorData.Properties);
                                    mergedExports.Add(export);
                                }
                            }

                            foreach (var editorExport in editorAsset.GetExports())
                            {
                                if (!mergedExports.Contains(editorExport))
                                {
                                    finalExports.Add(editorExport);
                                }
                            }
                        }
                        mergedExports.Clear();

                        // Serialize object, and return it indented
                        return new OkObjectResult(JsonConvert.SerializeObject(finalExports, Formatting.Indented));

                    case false:
                        UTexture2D TextureObject = (UTexture2D)Provider.LoadObject(path);
                        
                        // Texture data
                        SKBitmap TextureData = TextureObject.Decode();
                        if (TextureData == null)
                            return StatusCode(500, value: new
                            {
                                errored = true,
                                exceptionstring = "Invalid texture data, exported as json",
                                jsonoutput = new { TextureObject }
                            });

                        return File(TextureData.Encode(SKEncodedImageFormat.Png, quality: 100).AsStream(), "image/png");
                }
            }
            catch (Exception exception)
            {
                if (exception.Message.StartsWith("There is no game file"))
                    return new NotFoundObjectResult(
                            new
                            {
                                errored = true,
                                note = "Unable to find package"
                            }
                        );
                else return new ConflictObjectResult(
                    new
                    {
                        errored = true,
                        note = exception.Message
                    }
                );
            }
        }
    }
}
