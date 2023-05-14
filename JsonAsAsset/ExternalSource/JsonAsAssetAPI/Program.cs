// JsonAsAsset API
using JsonAsAssetAPI.Controllers;
using Microsoft.EntityFrameworkCore;

Console.Title = "Local Fetch API";
Console.WriteLine("Contributors: \n * [Tector]\n * [GMatrix]\n");

var builder = WebApplication.CreateBuilder(args);

Console.WriteLine("[core] Initializing globals, and provider..");

var globals = new Globals();
await globals.Initialize();

Console.WriteLine("[core] Initialized provider successfully\n");

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

app.Run();