//
// PyShell.xaml.h
// Declaration of the PyShell.xaml class.
//

#pragma once

#include "pch.h"
#include "PyShell.g.h"
#include "Python.h"
using namespace Platform;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Documents;
using namespace Windows::UI::ApplicationSettings;

namespace python34app
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class PyShell sealed
	{
            String ^command;
            int _prompt; // 1: ps1, 2: ps2
            String ^prompt();
            Run^ current_run;
            Paragraph^ current_paragraph;
            TextBox^ current_input;
            bool last_was_stderr;

            double orig_height; // for soft keyboard

            Platform::Collections::Vector<String^> ^history;
            unsigned int histpos;

            Windows::UI::Color forecolor, backcolor;

            bool running;
            Windows::UI::Core::CoreDispatcher ^dispatcher;

            Array<unsigned char> ^simple_string;
            void run_simple_string();

            PyObject *current_code;
            void run_code();

            PyObject* try_compile();
            PyObject *metrosetup;
            bool exited;
            void RunAsync(void (PyShell::*)(void));
            void StartInterpreter();
            void StopInterpreter();
            void AddText(String^ data, bool is_stderr = false);
            void AddTextAsync(String^ data, bool is_stderr = false);
	public:
            PyShell();

            void AddOutAsync(wchar_t *data);
            void AddErrAsync(wchar_t *data);
            void exit();

            //void get_settings_commands(SettingsPane ^p, SettingsPaneCommandsRequestedEventArgs^ args);

            void set_forecolor(Windows::UI::Color color);
            void set_backcolor(Windows::UI::Color color);
            void load_settings(Windows::Storage::ApplicationData^ appdata, Object^ o);
            void save_settings();

	protected:
            virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
	private:
            void KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
            void Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
            void SizeChanged(Platform::Object^, Windows::UI::Xaml::SizeChangedEventArgs^);
            void do_restart(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
            void run_file(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
            void enable_input();
            void disable_input();
	};
}
