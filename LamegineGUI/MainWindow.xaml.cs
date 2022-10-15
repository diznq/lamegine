using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Runtime.InteropServices;
using System.Threading;
using System.Diagnostics;
using System.Windows.Interop;
using WebSocketSharp;
using Newtonsoft.Json;
using System.IO;

namespace LamegineGUI
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        Process engine;
        WebSocket comm;

        public class Vector3
        {
            float[] vals;
            public Vector3(dynamic x, dynamic y, dynamic z)
            {
                vals = new float[3] { (float)(x), (float)(y), (float)(z) };
            }
            public Vector3(float x, float y, float z)
            {
                vals = new float[3] { x, y, z };
            }
            public override String ToString()
            {
                return "{" + vals[0] + "; " + vals[1] + "; " + vals[2] + "}";
            }
        }
        public class Vector4
        {
            float[] vals;
            public Vector4(dynamic x, dynamic y, dynamic z, dynamic w)
            {
                vals = new float[4] { (float)(x), (float)(y), (float)(z), (float)(w) };
            }
            public Vector4(float x, float y, float z, float w)
            {
                vals = new float[4] { x, y, z, w };
            }
            public override String ToString()
            {
                return "{" + vals[0] + "; " + vals[1] + "; " + vals[2] + "; "+vals[3]+"}";
            }
        }

        public class KV
        {
            public string Key { get; set; }
            public string Value { get; set; }
        }

        public class IObject3D
        {
            public uint id;
            public string name;
            public string className;
            public Vector3 pos;
            public Vector4 rotation;
            public Vector4 internal_rotation;
            public Vector3 scale;
            public bool animated;
            public bool physics;
            public IObject3D(dynamic obj)
            {
                try
                {
                    id = obj.id;
                    name = obj.name;
                    className = obj["class"];
                    animated = obj.animated;
                    physics = obj.physics;
                    pos = new Vector3(obj.pos[0], obj.pos[1], obj.pos[2]);
                    scale = new Vector3(obj.scale[0], obj.scale[1], obj.scale[2]);
                    rotation = new Vector4(obj.rotation[0], obj.rotation[1], obj.rotation[2], obj.rotation[3]);
                    internal_rotation = new Vector4(obj.internal_rotation[0], obj.internal_rotation[1], obj.internal_rotation[2], obj.internal_rotation[3]);
                } catch(Exception ex)
                {
                    MessageBox.Show(ex.ToString());
                }
            }

            public KV Get(string w, object v)
            {
                return new KV { Key = w, Value = v.ToString() };
            }
            public void ToListView(ListView view)
            {
                try
                {
                    view.Items.Clear();
                    view.Items.Add(Get("id", id));
                    view.Items.Add(Get("id", name));
                    view.Items.Add(Get("class", className));
                    view.Items.Add(Get("position", pos));
                    view.Items.Add(Get("rotation", rotation));
                    view.Items.Add(Get("internal_rotation", internal_rotation));
                    view.Items.Add(Get("scale", scale));
                    view.Items.Add(Get("physics", physics));
                    view.Items.Add(Get("animated", animated));
                } catch(Exception ex)
                {
                    MessageBox.Show(ex.ToString());
                }
            }
        }

        IObject3D activeObject;

        [DllImport("user32.dll")]
        private static extern int SetWindowLong(IntPtr hWnd, int nIndex, int dwNewLong);

        [DllImport("user32.dll", SetLastError = true)]
        private static extern int GetWindowLong(IntPtr hWnd, int nIndex);

        [DllImport("user32")]
        private static extern IntPtr SetParent(IntPtr hWnd, IntPtr hWndParent);

        [DllImport("user32")]
        private static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, int uFlags);

        private const int SWP_NOZORDER = 0x0004;
        private const int SWP_NOACTIVATE = 0x0010;
        private const int GWL_STYLE = -16;
        private const int WS_CAPTION = 0x00C00000;
        private const int WS_THICKFRAME = 0x00040000;

        IntPtr panelHandle;
        string selectedPath;

        public MainWindow()
        {
            InitializeComponent();
        }

        public string GetCwd()
        {
            return @"C:\Users\ja\source\repos\Lamegine\x64\Release\";
        }

        private void ListDirectory(TreeView treeView, string path)
        {
            treeView.Items.Clear();
            var rootDirectoryInfo = new DirectoryInfo(path);
            treeView.Items.Add(CreateDirectoryNode(rootDirectoryInfo));
        }

        private TreeViewItem CreateDirectoryNode(DirectoryInfo directoryInfo)
        {
            var directoryNode = new TreeViewItem();
            directoryNode.Header = directoryInfo.Name;
            foreach (var directory in directoryInfo.GetDirectories())
                directoryNode.Items.Add(CreateDirectoryNode(directory));
            foreach (var file in directoryInfo.GetFiles())
            {
                if (file.Name.EndsWith(".obj") || file.Name.EndsWith(".lmm") || file.Name.EndsWith(".dae") || file.Name.EndsWith(".ogg"))
                {
                    TreeViewItem item = new TreeViewItem();
                    item.Header = file.Name;
                    item.Selected += (s, a) =>
                    {
                        selectedPath = file.FullName.Substring(GetCwd().Length);
                        string cmd = JsonConvert.SerializeObject(new Commands.Command(new Commands.SetModelPath(selectedPath)));
                        comm.Send(cmd);
                    };
                    directoryNode.Items.Add(item);
                }
            }
            return directoryNode;
        }

        private void ResizeEmbeddedApp()
        {
            if (engine == null)
                return;

            Size size = GetElementPixelSize(engineWindow);

            SetWindowPos(engine.MainWindowHandle, IntPtr.Zero, 0, 0, (int)size.Width, (int)size.Height, SWP_NOZORDER | SWP_NOACTIVATE);
        }

        public Size GetElementPixelSize(UIElement element)
        {
            return new Size(1280 * 1.25, 720 * 1.25);
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {

            System.Windows.Forms.Integration.WindowsFormsHost host = new System.Windows.Forms.Integration.WindowsFormsHost();
            ProcessStartInfo psi = new ProcessStartInfo(GetCwd()+"Lamegine.exe");
            
            Size size = GetElementPixelSize(engineWindow);

            psi.Arguments = "-w "+((int)size.Width)+" -h "+((int)size.Height)+" -game Editor.dll -editor 1 -fs 0 -silent 1";
            psi.WorkingDirectory = GetCwd();
            
            engine = Process.Start(psi);

            while (engine.MainWindowHandle == IntPtr.Zero)
            {
                Thread.Sleep(1);
            }
            
            System.Windows.Forms.Panel panel = new System.Windows.Forms.Panel();
            panel.Width = (int)size.Width;
            panel.Height = (int)size.Height;
            engineWindow.Child = panel;
            panelHandle = panel.Handle;
            SetParent(engine.MainWindowHandle, panelHandle);

            int style = GetWindowLong(engine.MainWindowHandle, GWL_STYLE);
            style = style & ~WS_CAPTION & ~WS_THICKFRAME;
            SetWindowLong(engine.MainWindowHandle, GWL_STYLE, style);

            // resize embedded application & refresh
            ResizeEmbeddedApp();
            ListDirectory(tree, GetCwd());

            comm = new WebSocket(url: "ws://localhost:7142/subscribe", onMessage: OnMessage, onOpen: OnOpen);
            comm.Connect();

        }

        public void ShowDetail()
        {
            Dispatcher.Invoke(() =>
            {
                if(activeObject == null)
                {
                    details.Items.Clear();
                } else
                {
                    //MessageBox.Show("Yo");
                    activeObject.ToListView(details);
                }
            });
        }

        public void Log(string text)
        {
           Dispatcher.Invoke(() =>
           {
               //engineOutput.AppendText(text + "\n");
           });
        }

        public Task OnOpen()
        {
            Log("Connected");
            comm.Send("{\"id\":1,\"data\":{\"action\":\"subscribe\"}}");
            return Task.FromResult(0);
        }

        public Task OnMessage(MessageEventArgs message)
        {
            dynamic obj = JsonConvert.DeserializeObject(message.Text.ReadToEnd());
            if(obj.id == 0 && obj.data != null)
            {
                if(obj.data.action == "object_selected")
                {
                    activeObject = new IObject3D(obj.data.value);
                    ShowDetail();
                } else if(obj.data.action == "object_unselected")
                {
                    activeObject = null;
                    ShowDetail();
                }
            }
            return Task.FromResult(0);
        }
    }
}
