using ErrorServer.Data;
using Microsoft.AspNetCore.Components;
using Microsoft.AspNetCore.Components.Web;
using System.Net;
using System.Net.Sockets;
using System.Text;

public class Program {

	public static string ConnectionStatusMarkup { get; set; } = "<b style='color:red'>NOT CONNECTED</b>";
	public static string ErrorLog { get; set; } = string.Empty;
	public static string EndpointAddress { get; } = "0.0.0.0:4848";
	public static bool AutoClear { get; set; } = true;

    public static void Main(string[] args)
	{

		var builder = WebApplication.CreateBuilder(args);

		// Add services to the container.
		builder.Services.AddRazorPages();
		builder.Services.AddServerSideBlazor();
		builder.Services.AddSingleton<WeatherForecastService>();

		var app = builder.Build();

		// Configure the HTTP request pipeline.
		if (!app.Environment.IsDevelopment())
		{
			app.UseExceptionHandler("/Error");
			// The default HSTS value is 30 days. You may want to change this for production scenarios, see https://aka.ms/aspnetcore-hsts.
			app.UseHsts();
		}

		app.UseHttpsRedirection();

		app.UseStaticFiles();

		app.UseRouting();

		app.MapBlazorHub();
		app.MapFallbackToPage("/_Host");

		Thread t = new Thread(AcceptConnectionThread);
		t.Name = "Mine";
		t.Start();
		app.Run();

	}

	private static void AcceptConnectionThread()
	{
		Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
		socket.Bind(IPEndPoint.Parse(EndpointAddress));
		socket.Listen(10);

		while (true)
		{
			try
			{
				ProcessConnection(socket.Accept());
				ConnectionStatusMarkup = "<b style='color:red'>NOT CONNECTED</b>";
			}
			catch { }
		}

	}

	private static void ProcessConnection(Socket client)
	{
		try
		{
			if (AutoClear)
			{
				ErrorLog = string.Empty;
			}
			ConnectionStatusMarkup = "<b style='color:green'>CONNECTED</b>";
			int readBytes = 0;
			ErrorLog += $"<b style='color:green;'>Connected At {DateTime.Now}</b><br/>";
			do
			{
				byte[] messageBuffer = new byte[1024 * 16];
				readBytes = client.Receive(messageBuffer);
				ErrorLog += Encoding.ASCII.GetString(messageBuffer, 0, readBytes);
			} while (readBytes > 0);
		}
		catch { }
	}

}