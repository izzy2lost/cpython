using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Navigation;
using Microsoft.Phone.Controls;
using Microsoft.Phone.Shell;
using System.Windows.Documents;
using System.Windows.Media;
using Microsoft.Live;
using python33app.Resources;

namespace python33app
{
    /* Apparently, this must be a separate class, else XAML has problems processing it. */
    class GUIHandler : python33.GUI
    {
        private MainPage mainPage;
        public GUIHandler(MainPage mainPage)
        {
            this.mainPage = mainPage;
        }

        public void AddErrAsync(string text)
        {
            mainPage.AddErrAsync(text);
        }

        public void AddOutAsync(string text)
        {
            mainPage.AddOutAsync(text);
        }
    }

    public partial class MainPage : PhoneApplicationPage
    {
        public static LiveConnectSession session;
        // Set after FileChooser has completed
        public static byte[] downloadedCode;

        private Paragraph current_paragraph;
        private Run current_run;
        private TextBox current_input;
        bool current_is_stderr;
        string command;
        python33.Interpreter py;

        public MainPage()
        {
            InitializeComponent();
            py = new python33.Interpreter(new GUIHandler(this));
            Prompt();
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
            if (downloadedCode != null)
            {
                HideInput();
                py.RunScript(downloadedCode);
                Prompt();
            }
        }

        protected override Size ArrangeOverride(Size finalSize)
        {
            var result = base.ArrangeOverride(finalSize);
            if (current_input != null)
                current_input.Width = ActualWidth * 0.8;
            return result;
        }

        private void NextRun(bool is_stderr)
        {
            current_paragraph = new Paragraph();
            if (is_stderr)
                current_paragraph.Foreground = new SolidColorBrush(Colors.Red);
            current_run = new Run();
            current_paragraph.Inlines.Add(current_run);
            Output.Blocks.Add(current_paragraph);
            current_is_stderr = is_stderr;
        }

        private void AddText(string text, bool is_stderr)
        {
            bool first = true;
            if (current_run == null || current_is_stderr != is_stderr)
                NextRun(is_stderr);
            foreach (var line in text.Split(new char[]{'\n'})) 
            {
                if (!first)
                {
                    NextRun(is_stderr);
                }
                current_run.Text += line;
                first = false;
            }
        }

        public void AddOutAsync(string s)
        {
            AddText(s, false);
        }

        public void AddErrAsync(string s)
        {
            AddText(s, true);
        }

        private void ShowInput()
        {
            if (current_input != null)
                return;
            current_input = new TextBox();
            current_input.FontFamily = Output.FontFamily;
            current_input.FontSize = Output.FontSize;
            current_input.KeyDown += current_input_KeyDown;
            // Set to 100 for now, adjust on 
            current_input.Width = 100;
            var container = new InlineUIContainer();
            container.Child = current_input;
            current_paragraph.Inlines.Add(container);
            scrollView.UpdateLayout();
            scrollView.ScrollToVerticalOffset(Output.ActualHeight);
        }
        
        void current_input_KeyDown(object sender, System.Windows.Input.KeyEventArgs e)
        {
            if (e.Key == System.Windows.Input.Key.Enter)
            {
                var line = current_input.Text;
                HideInput();
                line += "\n";
                AddOutAsync(line);
                command += line;    
                python33.Object result = py.TryCompile(command);
                if (result == null)
                {
                    command = null;
                }
                else if (!result.isNone)
                {
                    command = null;
                    py.RunCode(result);
                }
                Prompt();
            }
        }

        private void HideInput()
        {
            if (current_input == null)
                return;
            current_paragraph.Inlines.RemoveAt(current_paragraph.Inlines.Count-1);
            current_input = null;
        }

        private void Prompt()
        {
            string prompt = command == null ? ">>> ":"... ";
            AddText(prompt, false);
            ShowInput();
        }

        private void Restart(object sender, EventArgs e)
        {
            py.ShutDown();
            Output.Blocks.Clear();
            current_run = null;
            current_is_stderr = false;
            current_input = null;
            py = new python33.Interpreter(new GUIHandler(this));
            Prompt();           
        }

        private void RunFile(object sender, EventArgs e)
        {
            this.NavigationService.Navigate(new Uri("/FileChooser.xaml", UriKind.Relative));
        }

        private void Output_ManipulationDelta_1(object sender, System.Windows.Input.ManipulationDeltaEventArgs e)
        {
            var scale = ((ScaleTransform)Output.RenderTransform);
            var cumul = e.CumulativeManipulation.Scale;
            double min, max;
            if (cumul.X > cumul.Y) {
                max = cumul.X;
                min = cumul.Y;
            } else {
                max = cumul.Y;
                min = cumul.X;
            }
            if (min > 1)
            {
                scale.ScaleX = scale.ScaleY = max;
            }
            else
            {
                scale.ScaleX = scale.ScaleY = min;
            }
            e.Handled = true;
        }
    }
}