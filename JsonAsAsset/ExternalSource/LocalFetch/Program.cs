// JsonAsAsset API
using Microsoft.EntityFrameworkCore;

var globals = new Globals();
Console.Title = "Local Fetch API";

await globals.Initialize();
globals.WriteLog("CORE", ConsoleColor.Green, "Initialized provider successfully");

var builder = WebApplication.CreateBuilder(args);
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

app.UseCors(b => b
    .AllowAnyOrigin()
    .AllowAnyMethod()
    .AllowAnyHeader());

app.UseHttpsRedirection();
app.UseAuthentication();
app.UseRouting();
app.UseAuthorization();
app.MapControllers();

globals.WriteLog("CORE", ConsoleColor.Green, "Running API...");
globals.WriteLog("CREDITS", ConsoleColor.DarkRed, "Created by Tector and GMatrix, thank you for using! :)");
app.Run();