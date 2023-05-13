using Microsoft.EntityFrameworkCore;
using CUE4Parse.FileProvider;
using CUE4Parse.UE4.Versions;
using CUE4Parse.Encryption.Aes;
using CUE4Parse.UE4.Objects.Core.Misc;
using CUE4Parse.MappingsProvider;

public class Globals
{
    public static DefaultFileProvider Provider;

    public async Task Initialize()
    {
        Globals.Provider = new DefaultFileProvider("G:/Epic Games/Fortnite/FortniteGame/Content/Paks", SearchOption.TopDirectoryOnly, true, new VersionContainer(EGame.GAME_UE5_3));
        Globals.Provider.Initialize();

        await Globals.Provider.SubmitKeyAsync(new FGuid(), new FAesKey("0x9BC4ED189BCC283B21AB2929CDF87EACFE0187DA71AF700D61AB4D8D08AAB862"));
        Globals.Provider.MappingsContainer = new FileUsmapTypeMappingsProvider("./mappings.usmap");

        Globals.Provider.LoadLocalization(ELanguage.English);
        Globals.Provider.LoadVirtualPaths();
    }

    static Globals()
    {
        Provider = new DefaultFileProvider("G:/Epic Games/Fortnite/FortniteGame/Content/Paks", SearchOption.TopDirectoryOnly, true, new VersionContainer(EGame.GAME_UE5_3));
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