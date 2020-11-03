//
// Settings.xaml.h
// Deklaration der Settings-Klasse
//

#pragma once

#include "Settings.g.h"
#include "PyShell.xaml.h"

namespace python34app
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class Settings sealed
	{
            PyShell ^shell;
	public:
		Settings(PyShell ^shell);
        private:
            void do_close(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
            void white_on_black(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
            void black_on_white(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        };
}
