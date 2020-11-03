using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Navigation;
using System.Threading.Tasks;
using System.IO;
using System.IO.IsolatedStorage;
using Microsoft.Phone.Controls;
using Microsoft.Phone.Shell;
using Microsoft.Live;
using Microsoft.Live.Controls;

namespace python33app
{
    class SkyDriveItem : ListBoxItem
    {
        public string id, name;
        public SkyDriveItem(string id, string name)
        {
            this.id = id;
            this.name = name;
            this.Content = name;
        }

        virtual public bool CanRun
        {
            get { return false; }
        }

        virtual public bool IsFolder
        {
            get { return false; }
        }

    }

    class SkyDriveFolder : SkyDriveItem
    {
        public SkyDriveFolder(string id, string name) : base(id, name)
        {
            this.Content = "[" + name + "]";
        }

        public override bool IsFolder
        {
            get
            {
                return true;
            }
        }
    }

    class SkyDriveFile : SkyDriveItem
    {
        public SkyDriveFile(string id, string name)
            : base(id, name)
        {
            IsEnabled = CanRun;
        }

        override public bool CanRun
        {
            get { return name.EndsWith(".py"); }
        }
    }

    /*
       <Grid Margin="10">
          <TextBlock>Name</TextBlock>
            <ListBox>
              <ListBoxItem>Folder</ListBoxItem>
            </ListBox>
       </Grid>

     */
    class FolderListing : Grid
    {
        FolderListBox box;
        string name;
        TextBlock heading;
        public FolderListing(string name)
        {
            this.name = name;
            RowDefinitions.Add(new RowDefinition());
            RowDefinitions.Add(new RowDefinition());

            Margin = new Thickness(0, 0, 10, 0);
            
            heading = new TextBlock();
            heading.Text = "[Loading]";
            Children.Add(heading);
            SetRow(heading, 0);
            
            box = new FolderListBox(this);
            Children.Add(box);
            SetRow(box, 1);
        }

        public ListBox Box
        {
            get { return box; }
        }

        public void LoadingComplete()
        {
            heading.Text = name;
        }
    }

    class FolderListBox : ListBox
    {
        public FolderListing Listing { get; private set; }
        public FolderListBox(FolderListing listing)
        {
            Listing = listing;
        }
    }

    public partial class FileChooser : PhoneApplicationPage
    {
        LiveConnectClient client;
        SkyDriveFile selected;

        public FileChooser()
        {
            InitializeComponent();
        }

        private async void btnSignin_SessionChanged(object sender, LiveConnectSessionChangedEventArgs e)
        {
            if (e.Status == LiveConnectSessionStatus.Connected)
            {
                MainPage.session = e.Session;
                client = new LiveConnectClient(MainPage.session);
                NavigationService.Navigate(new Uri("/FileChooser.xaml", UriKind.Relative));
                await AddDirectory("me/skydrive", "SkyDrive");
            }
            else
            {
                MainPage.session = null;
            }
        }

        private async Task AddDirectory(string id, string name)
        {
            try
            {
                var folder = new FolderListing(name);
                Folders.Children.Add(folder);
                LiveOperationResult operationResult = await client.GetAsync("/" + id + "/files");
                dynamic result = operationResult.Result;
                if (result.data == null)
                {
                    this.ShowError("Server did not return a valid response.");
                    return;
                }

                dynamic items = result.data;
                folder.Box.SelectionChanged += Box_SelectionChanged;
                foreach (dynamic item in items)
                {
                    SkyDriveItem e;
                    switch ((string)item.type)
                    {
                        case "folder":
                        case "album":
                            e = new SkyDriveFolder(item.id, item.name);
                            break;
                        default:
                            e = new SkyDriveFile(item.id, item.name);
                            break;
                    }
                    folder.Box.Items.Add(e);
                }
                folder.LoadingComplete();
            }
            catch (LiveConnectException e)
            {
                this.ShowError(e.Message);
            }
        }

        async void Box_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var listbox = (FolderListBox)sender;
            var panel = listbox.Listing;
            int pos = Folders.Children.IndexOf(panel);
            /* Delete every subsequent folder */
            while (Folders.Children.Count > pos + 1)
                Folders.Children.RemoveAt(pos + 1);
            /* Analyze selected item if any */
            if (e.AddedItems.Count == 0)
                return;
            SkyDriveItem item = (SkyDriveItem)e.AddedItems[0];
            RunButton.IsEnabled = item.CanRun;
            selected = item.CanRun ? (SkyDriveFile)item : null;
            if (item.IsFolder)
            {
                await AddDirectory(item.id, item.name);
            }
        }

        private async void OnLoaded(object sender, RoutedEventArgs routedEventArgs)
        {
            Loaded -= OnLoaded;

            if (MainPage.session == null)
            {
                this.NavigationService.Navigate(new Uri("/SkyDriveLogin.xaml", UriKind.Relative));
                return;
            }

            client = new LiveConnectClient(MainPage.session);

            await AddDirectory("me/skydrive", "SkyDrive");
        }

        void ShowError(string msg)
        { }

        private async void RunSelectedFile(object sender, RoutedEventArgs e)
        {
            var result = await client.DownloadAsync("/"+selected.id+"/content");
            int len = (int)result.Stream.Length;
            // allow for zero-termination
            var buf = new byte[len+1];
            int read = await result.Stream.ReadAsync(buf, 0, len);
            MainPage.downloadedCode = buf;
            NavigationService.GoBack();
        }

    }
}