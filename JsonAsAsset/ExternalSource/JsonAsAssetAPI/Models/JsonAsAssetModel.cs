using Microsoft.EntityFrameworkCore;
using CUE4Parse.FileProvider;
using CUE4Parse.UE4.Versions;
using CUE4Parse.Encryption.Aes;
using CUE4Parse.UE4.Objects.Core.Misc;
using CUE4Parse.MappingsProvider;
using UE4Config.Parsing;
using CUE4Parse.Utils;
using Serilog.Core;

public class Globals
{
    public static DefaultFileProvider? Provider;

    public static string? MappingFilePath;
    public static string? ArchiveDirectory;
    public static string? UnrealVersion;
    public static string? ArchiveKey;

    public void PropagateSettings(ConfigIni config)
    {
        var values = new List<string>();
        {
            config.EvaluatePropertyValues("/Script/JsonAsAsset.JsonAsAssetSettings", "MappingFilePath", values);
        }
        MappingFilePath = values[0].SubstringBeforeLast("\"").SubstringAfterLast("\"");
        {
            config.EvaluatePropertyValues("/Script/JsonAsAsset.JsonAsAssetSettings", "ArchiveDirectory", values);
        }
        ArchiveDirectory = values[0].SubstringBeforeLast("\"").SubstringAfterLast("\"");
        {
            config.EvaluatePropertyValues("/Script/JsonAsAsset.JsonAsAssetSettings", "UnrealVersion", values);
        }
        UnrealVersion = values[0];
        {
            config.EvaluatePropertyValues("/Script/JsonAsAsset.JsonAsAssetSettings", "ArchiveKey", values);
        }
        ArchiveKey = values[0];
    }

    // Find config folder
    public ConfigIni GetEditorConfig()
    {
        string config_folder = System.AppDomain.CurrentDomain.BaseDirectory.SubstringBeforeLast("\\Plugins\\") + "\\Config\\";
        ConfigIni config = new ConfigIni("DefaultEditorPerProjectUserSettings");
        config.Read(File.OpenText(config_folder + "DefaultEditorPerProjectUserSettings.ini"));

        return config;
    }

    public async Task Initialize()
    {
        // Find config folder
        string config_folder = System.AppDomain.CurrentDomain.BaseDirectory.SubstringBeforeLast("\\Plugins\\") + "\\Config\\";
        Console.WriteLine("Found config folder: " + config_folder.SubstringBeforeLast("\\") + "\n");

        // DefaultEditorPerProjectUserSettings
        ConfigIni config = GetEditorConfig();
        PropagateSettings(config);

        Provider = new DefaultFileProvider(ArchiveDirectory, SearchOption.TopDirectoryOnly, true, new VersionContainer((EGame)Enum.Parse(typeof(EGame), UnrealVersion, true)));
        Provider.Initialize();
        await Provider.SubmitKeyAsync(new FGuid(), new FAesKey(ArchiveKey));

        // Get AES Keys
        var values = new List<string>();
        config.EvaluatePropertyValues("/Script/JsonAsAsset.JsonAsAssetSettings", "AESKeys", values);

        foreach (string key in values) {
            // Isolate entries into 2
            var _key = key; var ReAssignedKey = _key.SubstringAfterLast("(").SubstringBeforeLast(")");
            string[] entries = ReAssignedKey.Split(",");

            // Key & Guid
            var Key = entries[0].SubstringBeforeLast("\"").SubstringAfterLast("\"");
            var Guid = entries[1].SubstringBeforeLast("\"").SubstringAfterLast("\"");

            await Provider.SubmitKeyAsync(new FGuid(Guid), new FAesKey(Key));
        }

        Provider.MappingsContainer = new FileUsmapTypeMappingsProvider(MappingFilePath);
        Provider.LoadLocalization(ELanguage.English);
        Provider.LoadVirtualPaths();
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