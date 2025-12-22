using ScaleFXStudio.Models;
using ScaleFXStudio.Services;

namespace ScaleFXStudio;

public partial class MainForm : Form
{
    private readonly ConfigService _configService;
    private ScaleFXConfiguration _config;
    private string? _currentFilePath;
    private bool _isDirty;

    private MenuStrip _menuStrip = null!;
    private ToolStrip _toolStrip = null!;
    private SplitContainer _splitContainer = null!;
    private TreeView _treeView = null!;
    private PropertyGrid _propertyGrid = null!;
    private StatusStrip _statusStrip = null!;
    private ToolStripStatusLabel _statusLabel = null!;

    public MainForm()
    {
        _configService = new ConfigService();
        _config = _configService.CreateDefault();
        
        InitializeComponent();
        SetupUI();
        PopulateTree();
        UpdateTitle();
    }

    private void InitializeComponent()
    {
        SuspendLayout();
        
        AutoScaleDimensions = new SizeF(7F, 15F);
        AutoScaleMode = AutoScaleMode.Font;
        ClientSize = new Size(1000, 700);
        MinimumSize = new Size(800, 500);
        StartPosition = FormStartPosition.CenterScreen;
        
        ResumeLayout(false);
    }

    private void SetupUI()
    {
        // Menu Strip
        _menuStrip = new MenuStrip();
        
        var fileMenu = new ToolStripMenuItem("&File");
        fileMenu.DropDownItems.Add("&New", null, OnNew);
        fileMenu.DropDownItems.Add("&Open...", null, OnOpen);
        fileMenu.DropDownItems.Add("&Save", null, OnSave);
        fileMenu.DropDownItems.Add("Save &As...", null, OnSaveAs);
        fileMenu.DropDownItems.Add(new ToolStripSeparator());
        fileMenu.DropDownItems.Add("E&xit", null, (s, e) => Close());
        
        var editMenu = new ToolStripMenuItem("&Edit");
        editMenu.DropDownItems.Add("&Reset to Defaults", null, OnResetDefaults);
        
        var helpMenu = new ToolStripMenuItem("&Help");
        helpMenu.DropDownItems.Add("&About", null, OnAbout);
        
        _menuStrip.Items.AddRange(new[] { fileMenu, editMenu, helpMenu });
        
        // Tool Strip
        _toolStrip = new ToolStrip();
        _toolStrip.Items.Add(new ToolStripButton("New", null, OnNew) { DisplayStyle = ToolStripItemDisplayStyle.Text });
        _toolStrip.Items.Add(new ToolStripButton("Open", null, OnOpen) { DisplayStyle = ToolStripItemDisplayStyle.Text });
        _toolStrip.Items.Add(new ToolStripButton("Save", null, OnSave) { DisplayStyle = ToolStripItemDisplayStyle.Text });
        _toolStrip.Items.Add(new ToolStripSeparator());
        _toolStrip.Items.Add(new ToolStripButton("Reset", null, OnResetDefaults) { DisplayStyle = ToolStripItemDisplayStyle.Text });
        
        // Split Container
        _splitContainer = new SplitContainer
        {
            Dock = DockStyle.Fill,
            SplitterDistance = 250,
            Panel1MinSize = 150,
            Panel2MinSize = 400
        };
        
        // Tree View
        _treeView = new TreeView
        {
            Dock = DockStyle.Fill,
            HideSelection = false,
            ShowLines = true,
            ShowPlusMinus = true,
            ShowRootLines = true
        };
        _treeView.AfterSelect += OnTreeNodeSelected;
        
        // Property Grid
        _propertyGrid = new PropertyGrid
        {
            Dock = DockStyle.Fill,
            PropertySort = PropertySort.Categorized,
            ToolbarVisible = true,
            HelpVisible = true
        };
        _propertyGrid.PropertyValueChanged += OnPropertyValueChanged;
        
        // Status Strip
        _statusStrip = new StatusStrip();
        _statusLabel = new ToolStripStatusLabel("Ready");
        _statusStrip.Items.Add(_statusLabel);
        
        // Layout
        _splitContainer.Panel1.Controls.Add(_treeView);
        _splitContainer.Panel2.Controls.Add(_propertyGrid);
        
        Controls.Add(_splitContainer);
        Controls.Add(_toolStrip);
        Controls.Add(_menuStrip);
        Controls.Add(_statusStrip);
        
        MainMenuStrip = _menuStrip;
    }

    private void PopulateTree()
    {
        _treeView.BeginUpdate();
        _treeView.Nodes.Clear();
        
        var root = new TreeNode("ScaleFX Configuration") { Tag = _config };
        
        if (_config.EngineFx != null)
        {
            var engineNode = new TreeNode("Engine FX") { Tag = _config.EngineFx };
            if (_config.EngineFx.EngineToggle != null)
                engineNode.Nodes.Add(new TreeNode("Engine Toggle") { Tag = _config.EngineFx.EngineToggle });
            if (_config.EngineFx.Sounds != null)
            {
                var soundsNode = new TreeNode("Sounds") { Tag = _config.EngineFx.Sounds };
                if (_config.EngineFx.Sounds.Starting != null)
                    soundsNode.Nodes.Add(new TreeNode("Starting") { Tag = _config.EngineFx.Sounds.Starting });
                if (_config.EngineFx.Sounds.Running != null)
                    soundsNode.Nodes.Add(new TreeNode("Running") { Tag = _config.EngineFx.Sounds.Running });
                if (_config.EngineFx.Sounds.Stopping != null)
                    soundsNode.Nodes.Add(new TreeNode("Stopping") { Tag = _config.EngineFx.Sounds.Stopping });
                if (_config.EngineFx.Sounds.Transitions != null)
                    soundsNode.Nodes.Add(new TreeNode("Transitions") { Tag = _config.EngineFx.Sounds.Transitions });
                engineNode.Nodes.Add(soundsNode);
            }
            root.Nodes.Add(engineNode);
        }
        
        if (_config.GunFx != null)
        {
            var gunNode = new TreeNode("Gun FX") { Tag = _config.GunFx };
            if (_config.GunFx.Trigger != null)
                gunNode.Nodes.Add(new TreeNode("Trigger") { Tag = _config.GunFx.Trigger });
            if (_config.GunFx.Smoke != null)
                gunNode.Nodes.Add(new TreeNode("Smoke") { Tag = _config.GunFx.Smoke });
            if (_config.GunFx.TurretControl != null)
            {
                var turretNode = new TreeNode("Turret Control") { Tag = _config.GunFx.TurretControl };
                if (_config.GunFx.TurretControl.Pitch != null)
                    turretNode.Nodes.Add(new TreeNode("Pitch Servo") { Tag = _config.GunFx.TurretControl.Pitch });
                if (_config.GunFx.TurretControl.Yaw != null)
                    turretNode.Nodes.Add(new TreeNode("Yaw Servo") { Tag = _config.GunFx.TurretControl.Yaw });
                gunNode.Nodes.Add(turretNode);
            }
            if (_config.GunFx.RateOfFire != null)
            {
                var ratesNode = new TreeNode("Rates of Fire") { Tag = _config.GunFx.RateOfFire };
                for (int i = 0; i < _config.GunFx.RateOfFire.Count; i++)
                {
                    var rate = _config.GunFx.RateOfFire[i];
                    ratesNode.Nodes.Add(new TreeNode($"Rate {i + 1}: {rate.Name ?? rate.Rpm.ToString()} RPM") { Tag = rate });
                }
                gunNode.Nodes.Add(ratesNode);
            }
            root.Nodes.Add(gunNode);
        }
        
        _treeView.Nodes.Add(root);
        _treeView.ExpandAll();
        _treeView.SelectedNode = root;
        _treeView.EndUpdate();
    }

    private void OnTreeNodeSelected(object? sender, TreeViewEventArgs e)
    {
        if (e.Node?.Tag != null)
        {
            _propertyGrid.SelectedObject = e.Node.Tag;
            _statusLabel.Text = $"Editing: {e.Node.Text}";
        }
    }

    private void OnPropertyValueChanged(object? sender, PropertyValueChangedEventArgs e)
    {
        _isDirty = true;
        UpdateTitle();
        
        // Refresh tree node names for rate of fire items
        if (_treeView.SelectedNode?.Tag is RateOfFireConfig rate)
        {
            _treeView.SelectedNode.Text = $"Rate: {rate.Name ?? rate.Rpm.ToString()} RPM";
        }
    }

    private void UpdateTitle()
    {
        var fileName = _currentFilePath != null ? Path.GetFileName(_currentFilePath) : "Untitled";
        var dirty = _isDirty ? " *" : "";
        Text = $"ScaleFX Studio - {fileName}{dirty}";
    }

    private void OnNew(object? sender, EventArgs e)
    {
        if (!ConfirmDiscard()) return;
        
        _config = _configService.CreateDefault();
        _currentFilePath = null;
        _isDirty = false;
        PopulateTree();
        UpdateTitle();
        _statusLabel.Text = "New configuration created";
    }

    private void OnOpen(object? sender, EventArgs e)
    {
        if (!ConfirmDiscard()) return;
        
        using var dialog = new OpenFileDialog
        {
            Filter = "YAML Files (*.yaml;*.yml)|*.yaml;*.yml|All Files (*.*)|*.*",
            Title = "Open Configuration File"
        };
        
        if (dialog.ShowDialog() == DialogResult.OK)
        {
            try
            {
                _config = _configService.Load(dialog.FileName);
                _currentFilePath = dialog.FileName;
                _isDirty = false;
                PopulateTree();
                UpdateTitle();
                _statusLabel.Text = $"Loaded: {dialog.FileName}";
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error loading file:\n{ex.Message}", "Error", 
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }

    private void OnSave(object? sender, EventArgs e)
    {
        if (_currentFilePath == null)
        {
            OnSaveAs(sender, e);
            return;
        }
        
        SaveToFile(_currentFilePath);
    }

    private void OnSaveAs(object? sender, EventArgs e)
    {
        using var dialog = new SaveFileDialog
        {
            Filter = "YAML Files (*.yaml)|*.yaml|All Files (*.*)|*.*",
            Title = "Save Configuration File",
            DefaultExt = "yaml",
            FileName = "config.yaml"
        };
        
        if (dialog.ShowDialog() == DialogResult.OK)
        {
            SaveToFile(dialog.FileName);
        }
    }

    private void SaveToFile(string filePath)
    {
        try
        {
            _configService.Save(filePath, _config);
            _currentFilePath = filePath;
            _isDirty = false;
            UpdateTitle();
            _statusLabel.Text = $"Saved: {filePath}";
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Error saving file:\n{ex.Message}", "Error", 
                MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private void OnResetDefaults(object? sender, EventArgs e)
    {
        if (MessageBox.Show("Reset all settings to defaults?", "Confirm Reset",
            MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
        {
            _config = _configService.CreateDefault();
            _isDirty = true;
            PopulateTree();
            UpdateTitle();
            _statusLabel.Text = "Configuration reset to defaults";
        }
    }

    private void OnAbout(object? sender, EventArgs e)
    {
        MessageBox.Show(
            "ScaleFX Studio Configuration Editor\n\n" +
            "Version 1.0\n\n" +
            "GUI editor for ScaleFX Hub scale model effects system configuration.\n\n" +
            "© 2025 MSB (Marcin Scale Builds)",
            "About",
            MessageBoxButtons.OK,
            MessageBoxIcon.Information);
    }

    private bool ConfirmDiscard()
    {
        if (!_isDirty) return true;
        
        var result = MessageBox.Show(
            "You have unsaved changes. Do you want to save before continuing?",
            "Unsaved Changes",
            MessageBoxButtons.YesNoCancel,
            MessageBoxIcon.Warning);
        
        if (result == DialogResult.Yes)
        {
            OnSave(null, EventArgs.Empty);
            return !_isDirty; // Returns true if save succeeded
        }
        
        return result == DialogResult.No;
    }

    protected override void OnFormClosing(FormClosingEventArgs e)
    {
        if (!ConfirmDiscard())
        {
            e.Cancel = true;
        }
        base.OnFormClosing(e);
    }
}
