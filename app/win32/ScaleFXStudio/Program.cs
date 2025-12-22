namespace ScaleFXStudio;

static class Program
{
    [STAThread]
    static void Main(string[] args)
    {
        ApplicationConfiguration.Initialize();
        
        var mainForm = new MainForm();
        
        // If a file path was passed as argument, open it
        if (args.Length > 0 && File.Exists(args[0]))
        {
            try
            {
                var configService = new Services.ConfigService();
                var config = configService.Load(args[0]);
                // Would need to pass this to MainForm - for now just start empty
            }
            catch
            {
                // Ignore load errors on startup
            }
        }
        
        Application.Run(mainForm);
    }
}
