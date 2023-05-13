using Microsoft.EntityFrameworkCore;
using CUE4Parse.FileProvider;
using CUE4Parse.UE4.Versions;
using CUE4Parse.Encryption.Aes;
using CUE4Parse.UE4.Objects.Core.Misc;
using CUE4Parse.MappingsProvider;
using UE4Config.Parsing;
using CUE4Parse.Utils;

public class Globals
{
    public static DefaultFileProvider? Provider;

    public async Task Initialize()
    {
        // Find config folder
        string config_folder = System.AppDomain.CurrentDomain.BaseDirectory.SubstringBeforeLast("\\Plugins\\") + "\\Config\\";
        ConfigIni config = new ConfigIni("DefaultEditorPerProjectUserSettings");

        // Read config file
        config.Read(File.OpenText(config_folder + "DefaultEditorPerProjectUserSettings.ini"));
        Console.WriteLine("Found config folder: " + config_folder.SubstringBeforeLast("\\") + "\n");

        var values = new List<string>();
        {
            config.EvaluatePropertyValues("/Script/JsonAsAsset.JsonAsAssetSettings", "MappingFilePath", values);
        }
        var MappingFilePath = values[0].SubstringBeforeLast("\"").SubstringAfterLast("\"");
        {
            config.EvaluatePropertyValues("/Script/JsonAsAsset.JsonAsAssetSettings", "ArchiveDirectory", values);
        }
        var ArchiveDirectory = values[0].SubstringBeforeLast("\"").SubstringAfterLast("\"");
        {
            config.EvaluatePropertyValues("/Script/JsonAsAsset.JsonAsAssetSettings", "UnrealVersion", values);
        }
        var UnrealVersion = values[0];
        {
            config.EvaluatePropertyValues("/Script/JsonAsAsset.JsonAsAssetSettings", "ArchiveKey", values);
        }
        var ArchiveKey = values[0];

        Console.WriteLine("Settings:");
        Console.WriteLine($" - Mappings: {MappingFilePath}");
        Console.WriteLine($" - Archive Directory: {ArchiveDirectory}");
        Console.WriteLine($" - Unreal Engine Version: {UnrealVersion}");
        Console.WriteLine($" - Archive Key: {ArchiveKey} \n");

        Globals.Provider = new DefaultFileProvider(ArchiveDirectory, SearchOption.TopDirectoryOnly, true, new VersionContainer((EGame)Enum.Parse(typeof(EGame), UnrealVersion, true)));
        Globals.Provider.Initialize();
        await Globals.Provider.SubmitKeyAsync(new FGuid(), new FAesKey(ArchiveKey));

        values = new List<string>();
        config.EvaluatePropertyValues("/Script/JsonAsAsset.JsonAsAssetSettings", "AESKeys", values);
        foreach (string key in values)
        {
            var t = key;
            var ReAssignedKey = t.SubstringAfterLast("(").SubstringBeforeLast(")");
            string[] entries = ReAssignedKey.Split(",");

            var Key = entries[0].SubstringBeforeLast("\"").SubstringAfterLast("\"");
            var Guid = entries[1].SubstringBeforeLast("\"").SubstringAfterLast("\"");

            await Globals.Provider.SubmitKeyAsync(new FGuid(Guid), new FAesKey(Key));
        }

        Globals.Provider.MappingsContainer = new FileUsmapTypeMappingsProvider(MappingFilePath);
        Globals.Provider.LoadLocalization(ELanguage.English);
        Globals.Provider.LoadVirtualPaths();
    }
}

namespace JsonAsAssetApi.Data
{
    public class ApiContext : DbContext
    {
        public ApiContext(DbContextOptions<ApiContext> options)
            : base(options)
        {
        }
    }
}