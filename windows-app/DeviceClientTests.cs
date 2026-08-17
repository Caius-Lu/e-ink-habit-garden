using System;

internal static class DeviceClientTests
{
    private static int failures;

    private static void Equal(string expected, string actual, string name)
    {
        if (expected != actual)
        {
            Console.Error.WriteLine("FAIL " + name + ": " + actual);
            failures++;
        }
        else Console.WriteLine("PASS " + name);
    }

    private static void Throws(Action action, string name)
    {
        try { action(); Console.Error.WriteLine("FAIL " + name); failures++; }
        catch (ArgumentException) { Console.WriteLine("PASS " + name); }
    }

    public static int Main()
    {
        Equal("http://einktodo.local", DeviceClient.NormalizeBaseUrl("einktodo.local/"), "hostname normalization");
        Equal("http://192.168.68.153/api/status", DeviceClient.BuildEndpoint("192.168.68.153", "/api/status"), "endpoint building");
        Equal("https://box.local", DeviceClient.NormalizeBaseUrl("https://box.local"), "https preserved");
        Throws(() => DeviceClient.NormalizeBaseUrl("ftp://bad"), "unsupported scheme rejected");
        Throws(() => DeviceClient.NormalizeBaseUrl("   "), "empty address rejected");
        return failures == 0 ? 0 : 1;
    }
}
