using ErrorWeb;
using Microsoft.AspNetCore.Components.Web;
using Microsoft.AspNetCore.Components.WebAssembly.Hosting;
using System.Net;
using System.Net.Sockets;

class Program {

	public static string ConnectionStatusMarkup { get; set; } = "<b style='color:green'>CONNECTED</b>";

	public static async void Main(string[] args)
    {
        var builder = WebAssemblyHostBuilder.CreateDefault(args);
        builder.RootComponents.Add<App>("#app");
        builder.RootComponents.Add<HeadOutlet>("head::after");

        builder.Services.AddScoped(sp => new HttpClient { BaseAddress = new Uri(builder.HostEnvironment.BaseAddress) });

        await builder.Build().RunAsync();
    }

	private static void AcceptConnectionThread()
	{
		Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
		socket.Bind(IPEndPoint.Parse("127.0.0.1:5000"));
		socket.Listen(10);

		while (true)
		{
			try
			{
				ConnectionStatusMarkup = "<b style='color:green'>CONNECTED</b>";
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
			while (client.Connected)
			{
				byte[] messageBuffer = new byte[1024 * 16];
				client.Receive(messageBuffer);
				Encoding.ASCII.GetString(messageBuffer);
				TextAreaErrorLog += messageBuffer;
				StateHasChanged();
			}
		}
		catch { }
	}

}