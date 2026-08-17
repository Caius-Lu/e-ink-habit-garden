using System;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using System.Web.Script.Serialization;

public sealed class HabitInfo
{
    public int index { get; set; }
    public string name { get; set; }
    public bool done { get; set; }
    public int streak { get; set; }
    public int total { get; set; }
}

public sealed class DeviceStatus
{
    public string name { get; set; }
    public int done { get; set; }
    public int total { get; set; }
    public int stage { get; set; }
    public string ip { get; set; }
    public HabitInfo[] habits { get; set; }
}

public sealed class DeviceClient : IDisposable
{
    private readonly HttpClient http;
    public string BaseUrl { get; private set; }

    public DeviceClient(string address)
    {
        BaseUrl = NormalizeBaseUrl(address);
        var handler = new HttpClientHandler { UseProxy = false };
        http = new HttpClient(handler) { Timeout = TimeSpan.FromSeconds(12) };
    }

    public static string NormalizeBaseUrl(string address)
    {
        address = (address ?? "").Trim().TrimEnd('/');
        if (address.Length == 0) throw new ArgumentException("设备地址不能为空");
        if (!address.Contains("://")) address = "http://" + address;
        Uri uri;
        if (!Uri.TryCreate(address, UriKind.Absolute, out uri) ||
            (uri.Scheme != Uri.UriSchemeHttp && uri.Scheme != Uri.UriSchemeHttps))
            throw new ArgumentException("只支持 HTTP 或 HTTPS 设备地址");
        return uri.GetLeftPart(UriPartial.Authority);
    }

    public static string BuildEndpoint(string address, string path)
    {
        return NormalizeBaseUrl(address) + "/" + (path ?? "").TrimStart('/');
    }

    public async Task<DeviceStatus> GetStatusAsync()
    {
        string json = await http.GetStringAsync(BuildEndpoint(BaseUrl, "/api/status"));
        return new JavaScriptSerializer().Deserialize<DeviceStatus>(json);
    }

    public Task<string> GetQuotesAsync()
    {
        return http.GetStringAsync(BuildEndpoint(BaseUrl, "/api/quotes"));
    }

    public async Task CheckInAsync(int habit)
    {
        string body = "habit=" + Uri.EscapeDataString(habit.ToString());
        var response = await http.PostAsync(BuildEndpoint(BaseUrl, "/api/checkin"),
            new StringContent(body, Encoding.UTF8, "application/x-www-form-urlencoded"));
        if (response.StatusCode != HttpStatusCode.OK && response.StatusCode != HttpStatusCode.Conflict)
            response.EnsureSuccessStatusCode();
    }

    public async Task SaveQuotesAsync(string quotes)
    {
        var response = await http.PostAsync(BuildEndpoint(BaseUrl, "/api/quotes"),
            new StringContent(quotes ?? "", Encoding.UTF8, "text/plain"));
        response.EnsureSuccessStatusCode();
    }

    public void Dispose() { http.Dispose(); }
}
