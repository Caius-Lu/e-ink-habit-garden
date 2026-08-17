using System;
using System.Drawing;
using System.IO;
using System.Threading.Tasks;
using System.Windows.Forms;

internal sealed class InkGardenForm : Form
{
    private readonly Color Paper = Color.FromArgb(244, 239, 224);
    private readonly Color Ink = Color.FromArgb(38, 53, 45);
    private readonly Color Leaf = Color.FromArgb(77, 108, 78);
    private readonly Color Clay = Color.FromArgb(185, 91, 62);
    private readonly TextBox address = new TextBox();
    private readonly Label connection = new Label();
    private readonly Label summary = new Label();
    private readonly FlowLayoutPanel habitList = new FlowLayoutPanel();
    private readonly TextBox quotes = new TextBox();
    private readonly Button saveQuotes = new Button();
    private DeviceClient client;

    private static string ConfigPath
    {
        get
        {
            string dir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "InkGarden");
            Directory.CreateDirectory(dir);
            return Path.Combine(dir, "device.txt");
        }
    }

    public InkGardenForm()
    {
        Text = "墨水盆景";
        ClientSize = new Size(940, 590);
        MinimumSize = new Size(820, 520);
        BackColor = Paper;
        ForeColor = Ink;
        Font = new Font("Microsoft YaHei UI", 10F);
        StartPosition = FormStartPosition.CenterScreen;

        var title = new Label { Text = "墨水盆景", Font = new Font("Microsoft YaHei UI", 26F, FontStyle.Bold), AutoSize = true, Location = new Point(30, 22), ForeColor = Ink };
        var subtitle = new Label { Text = "把认真生活的日子，养成一株看得见的植物。", AutoSize = true, Location = new Point(34, 72), ForeColor = Color.FromArgb(108, 121, 111) };
        Controls.Add(title);
        Controls.Add(subtitle);

        address.Location = new Point(565, 30);
        address.Size = new Size(225, 30);
        address.Text = File.Exists(ConfigPath) ? File.ReadAllText(ConfigPath).Trim() : "http://einktodo.local";
        Controls.Add(address);
        var connect = MakeButton("连接", Leaf, new Point(800, 28), new Size(105, 34));
        connect.Click += async (s, e) => await RefreshAllAsync(true);
        Controls.Add(connect);
        connection.Location = new Point(565, 68);
        connection.Size = new Size(340, 22);
        connection.ForeColor = Color.FromArgb(108, 121, 111);
        Controls.Add(connection);

        var leftCard = MakeCard(new Point(30, 112), new Size(420, 440));
        var rightCard = MakeCard(new Point(470, 112), new Size(435, 440));
        Controls.Add(leftCard);
        Controls.Add(rightCard);

        var today = new Label { Text = "今日浇水", Font = new Font("Microsoft YaHei UI", 17F, FontStyle.Bold), AutoSize = true, Location = new Point(22, 20), ForeColor = Ink };
        summary.Location = new Point(24, 58);
        summary.Size = new Size(365, 28);
        summary.ForeColor = Clay;
        summary.Font = new Font("Microsoft YaHei UI", 11F, FontStyle.Bold);
        habitList.Location = new Point(20, 96);
        habitList.Size = new Size(380, 270);
        habitList.FlowDirection = FlowDirection.TopDown;
        habitList.WrapContents = false;
        leftCard.Controls.Add(today);
        leftCard.Controls.Add(summary);
        leftCard.Controls.Add(habitList);
        var refresh = MakeButton("刷新状态", Ink, new Point(22, 382), new Size(115, 36));
        refresh.Click += async (s, e) => await RefreshAllAsync(false);
        leftCard.Controls.Add(refresh);

        var quoteTitle = new Label { Text = "名言花笺", Font = new Font("Microsoft YaHei UI", 17F, FontStyle.Bold), AutoSize = true, Location = new Point(22, 20), ForeColor = Ink };
        var quoteHint = new Label { Text = "一行一条；保存后墨水屏会自动刷新。", AutoSize = true, Location = new Point(24, 59), ForeColor = Color.FromArgb(108, 121, 111) };
        quotes.Location = new Point(24, 92);
        quotes.Size = new Size(387, 275);
        quotes.Multiline = true;
        quotes.ScrollBars = ScrollBars.Vertical;
        quotes.AcceptsReturn = true;
        quotes.BackColor = Color.FromArgb(255, 253, 246);
        quotes.ForeColor = Ink;
        quotes.BorderStyle = BorderStyle.FixedSingle;
        saveQuotes.Text = "保存名言";
        saveQuotes.Location = new Point(24, 382);
        saveQuotes.Size = new Size(115, 36);
        saveQuotes.BackColor = Clay;
        saveQuotes.ForeColor = Color.White;
        saveQuotes.FlatStyle = FlatStyle.Flat;
        saveQuotes.FlatAppearance.BorderSize = 0;
        saveQuotes.Click += async (s, e) => await SaveQuotesAsync();
        rightCard.Controls.Add(quoteTitle);
        rightCard.Controls.Add(quoteHint);
        rightCard.Controls.Add(quotes);
        rightCard.Controls.Add(saveQuotes);

        Shown += async (s, e) => await RefreshAllAsync(true);
        FormClosed += (s, e) => { if (client != null) client.Dispose(); };
    }

    private Panel MakeCard(Point location, Size size)
    {
        return new Panel { Location = location, Size = size, BackColor = Color.FromArgb(255, 253, 246), BorderStyle = BorderStyle.FixedSingle };
    }

    private Button MakeButton(string text, Color color, Point location, Size size)
    {
        var b = new Button { Text = text, BackColor = color, ForeColor = Color.White, FlatStyle = FlatStyle.Flat, Location = location, Size = size, Cursor = Cursors.Hand };
        b.FlatAppearance.BorderSize = 0;
        return b;
    }

    private DeviceClient CurrentClient()
    {
        string normalized = DeviceClient.NormalizeBaseUrl(address.Text);
        if (client == null || client.BaseUrl != normalized)
        {
            if (client != null) client.Dispose();
            client = new DeviceClient(normalized);
            File.WriteAllText(ConfigPath, normalized);
        }
        return client;
    }

    private async Task RefreshAllAsync(bool loadQuotes)
    {
        try
        {
            connection.Text = "正在连接…";
            DeviceClient dc = CurrentClient();
            DeviceStatus status = await dc.GetStatusAsync();
            connection.Text = "已连接 · " + status.ip;
            summary.Text = status.done + "/3 已完成 · 成长值 " + status.total + " 滴 · 第 " + status.stage + " 阶段";
            RenderHabits(status);
            if (loadQuotes) quotes.Text = (await dc.GetQuotesAsync()).Replace("\n", Environment.NewLine);
        }
        catch (Exception ex)
        {
            connection.Text = "连接失败";
            MessageBox.Show("无法连接墨水盆景：\n" + ex.Message + "\n\n请确认电脑与设备在同一个 Wi-Fi。", "连接失败", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }
    }

    private void RenderHabits(DeviceStatus status)
    {
        habitList.Controls.Clear();
        if (status.habits == null) return;
        foreach (HabitInfo habit in status.habits)
        {
            var row = new Panel { Width = 370, Height = 78, Margin = new Padding(0, 0, 0, 8), BackColor = Color.FromArgb(248, 245, 235) };
            var name = new Label { Text = (habit.done ? "●  " : "○  ") + habit.name, Location = new Point(14, 13), Size = new Size(210, 25), Font = new Font("Microsoft YaHei UI", 12F, FontStyle.Bold), ForeColor = habit.done ? Leaf : Ink };
            var streak = new Label { Text = "连续 " + habit.streak + " 天 · 累计 " + habit.total + " 次", Location = new Point(16, 45), Size = new Size(240, 22), ForeColor = Color.FromArgb(108, 121, 111) };
            row.Controls.Add(name);
            row.Controls.Add(streak);
            if (!habit.done)
            {
                int index = habit.index;
                var check = MakeButton("浇水", Leaf, new Point(278, 20), new Size(76, 36));
                check.Click += async (s, e) => { await CurrentClient().CheckInAsync(index); await RefreshAllAsync(false); };
                row.Controls.Add(check);
            }
            else
            {
                row.Controls.Add(new Label { Text = "今日完成", Location = new Point(287, 30), AutoSize = true, ForeColor = Leaf });
            }
            habitList.Controls.Add(row);
        }
    }

    private async Task SaveQuotesAsync()
    {
        try
        {
            saveQuotes.Enabled = false;
            await CurrentClient().SaveQuotesAsync(quotes.Text.Replace("\r\n", "\n"));
            MessageBox.Show("名言已保存，墨水屏正在刷新。", "保存成功", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }
        catch (Exception ex)
        {
            MessageBox.Show("保存失败：" + ex.Message, "保存失败", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }
        finally { saveQuotes.Enabled = true; }
    }
}

internal static class Program
{
    [STAThread]
    private static void Main()
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        Application.Run(new InkGardenForm());
    }
}
