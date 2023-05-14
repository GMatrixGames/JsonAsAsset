// JsonAsAsset API
using JsonAsAssetApi.Data;
using Microsoft.EntityFrameworkCore;

Console.WriteLine("JsonAsAsset API v.1.0.0\nAPI to allow asset downloading at runtime, for JsonAsAsset.\n");
Console.WriteLine("Contributors: \n * [Tector]\n * [GMatrix]\n\n> Also credits to the people that worked on CUE4Parse!");

var builder = WebApplication.CreateBuilder(args);

var glob = new Globals();
await glob.Initialize();

builder.Services.AddDbContext<ApiContext>
    (opt => opt.UseInMemoryDatabase("JsonAsAsset"));
builder.Services.AddControllers();
builder.Services.AddEndpointsApiExplorer();

builder.Services.AddHsts(options =>
{
    options.Preload = true;
    options.IncludeSubDomains = true;
    options.MaxAge = TimeSpan.FromDays(1);
});

var app = builder.Build();

if (!app.Environment.IsDevelopment())
{
    app.UseExceptionHandler("/Error");
    app.UseHsts();
}

app.UseHttpsRedirection();
app.UseAuthentication();
app.UseRouting();
app.UseAuthorization();
app.MapControllers();

app.Run();