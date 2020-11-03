//
// Privacy.xaml.cpp
// Implementierung der Klasse Privacy
//

#include "pch.h"
#include "Privacy.xaml.h"

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

Privacy::Privacy()
{
	InitializeComponent();
}

void Privacy::do_close(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto popup = (Popup^)Parent;
    popup->IsOpen = false;
}

