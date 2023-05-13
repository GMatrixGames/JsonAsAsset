using Microsoft.AspNetCore.Mvc;
using JsonAsAssetApi.Data;
using CUE4Parse.FileProvider;
using CUE4Parse.UE4.Assets.Exports;
using Newtonsoft.Json;

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
        public ObjectResult Get(bool raw, string path)
        {
            // Try to load object, if failed, return message
            try
            {
                Provider.LoadAllObjects(path);
            } 
            catch(Exception exception)
            {
                return new NotFoundObjectResult(
                        new {
                            errored = true,
                            note = 
                                exception.Message.StartsWith("There is no game file") 
                                    ? "Unable to find package" : 
                                    exception.Message
                        }
                    );
            }

            // if (raw)

            // Credit to MoutainFlash:
            //  - https://gist.github.com/MinshuG/55f0da93fb839d41050e634b288e81b1
            //    : (merging editor only data)
            var exports = Provider.LoadAllObjects(path);
            var finalExports = new List<UObject>();
            finalExports.AddRange(exports);

            var mergedExports = new List<UObject>();
            if (Provider.TryLoadPackage(path, out var editorAsset))
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

            return new OkObjectResult(JsonConvert.SerializeObject(finalExports, Formatting.Indented));
        }
    }
}
