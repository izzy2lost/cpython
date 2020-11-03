//
// Settings.xaml.cpp
// Implementierung der Klasse Settings
//

#include "pch.h"
#include "Settings.xaml.h"

using namespace python34app;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// Die Elementvorlage "Benutzersteuerelement" ist unter http://go.microsoft.com/fwlink/?LinkId=234236 dokumentiert.

Settings::Settings(PyShell ^s):
    shell(s)
{
	InitializeComponent();
}


void python34app::Settings::do_close(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto popup = (Popup^)Parent;
    popup->IsOpen = false;
}


void python34app::Settings::white_on_black(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    shell->set_forecolor(Windows::UI::Colors::White);
    shell->set_backcolor(Windows::UI::Colors::Black);
    shell->save_settings();
}


void python34app::Settings::black_on_white(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    shell->set_forecolor(Windows::UI::Colors::Black);
    shell->set_backcolor(Windows::UI::Colors::White);
    shell->save_settings();
}
