// JsonAsAsset API
using JsonAsAssetAPI.Controllers;
using Microsoft.EntityFrameworkCore;

var globals = new Globals();

Console.Title = "Local Fetch API";
Console.WriteLine("Contributors: \n * [Tector]\n * [GMatrix]\n");

var builder = WebApplication.CreateBuilder(args);
globals.WriteLog("CORE", ConsoleColor.Green, "Initializing globals, and provider..");

await globals.Initialize();
globals.WriteLog("CORE", ConsoleColor.Green, "Initialized provider successfully");

builder.Services.AddDbContext<DbContext>
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

app.UseCors(builder => builder
    .AllowAnyOrigin()
    .AllowAnyMethod()
    .AllowAnyHeader());

app.UseHttpsRedirection();
app.UseAuthentication();
app.UseRouting();
app.UseAuthorization();
app.MapControllers();

globals.WriteLog("CORE", ConsoleColor.Green, "Running API...");

app.Run();