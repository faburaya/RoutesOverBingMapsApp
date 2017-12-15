//
// MyWaypointControl.xaml.cpp
// Implementation of the MyWaypointControl class
//

#include "pch.h"
#include "MyWaypointControl.xaml.h"
#include "TheMap.h"

using namespace Windows::UI::Xaml;
using namespace Windows::Services::Maps;

using Windows::Foundation::Collections::IVectorView;

using namespace RoutesOverBingMapsApp;


// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

////////////////////////////
// MyWaypointControl Class
////////////////////////////

IObservableVector<WayptInputComboOpt ^> ^ MyWaypointControl::optionsOfInput = nullptr;

/// <summary>
/// Initializes a new instance of the <see cref="MyWaypointControl"/> class.
/// </summary>
MyWaypointControl::MyWaypointControl()
{
	InitializeComponent();

    /* Some waypoint data needs to be provided here because the bindings
       upon initialization need to have something to show in the control.
       Later, when the actual waypoint data is passed via XAML property,
       those bindings have to be refreshed. */
    m_waypoint = ref new Waypoint(0, ref new Geopoint(BasicGeoposition{ 0 }));

    m_optionOfInput = WaypointInputOption::Address;

    if (optionsOfInput == nullptr)
    {
        optionsOfInput = ref new Platform::Collections::Vector<WayptInputComboOpt ^>();
        optionsOfInput->Append(ref new WayptInputComboOpt(WaypointInputOption::Address, Platform::StringReference(L"Address")));
        optionsOfInput->Append(ref new WayptInputComboOpt(WaypointInputOption::Coordinates, Platform::StringReference(L"Coordinates")));
        optionsOfInput->Append(ref new WayptInputComboOpt(WaypointInputOption::Pointer, Platform::StringReference(L"Pointer")));
    }
}


/// <summary>
/// Should be called whenever input is changed in any of the
/// text boxes for waypoint data (address & coordinates).
/// </summary>
void MyWaypointControl::OnTextChangedInput()
{
    // current input location has already been verified by geocoding?
    if (m_waypoint->IsVerified)
        return;

    static auto defBgBrush = (Media::Brush ^)this->Resources->Lookup(ref new Platform::String(L"BgColorInitialState"));
    
    // reset the appearance of the control:
    addressTextBox->Background = defBgBrush;
    latitudeTextBox->Background = defBgBrush;
    longitudeTextBox->Background = defBgBrush;
}


/// <summary>
/// Called when this user control has its data context changed.
/// </summary>
/// <remarks>
/// Change of data context happens whenever this instance is reused (after
/// being deleted from a ListView, for example) and must rebind to a new view
/// model object. In this scenario, the state of the child controls must be
/// reset to their initial visual states.
/// </remarks>
/// <param name="sender">The sender (user control).</param>
/// <param name="evArgs">The event arguments.</param>
void MyWaypointControl::OnDataContextChanged(FrameworkElement ^sender, DataContextChangedEventArgs ^evArgs)
{
    auto newViewModel = safe_cast<Waypoint ^> (evArgs->NewValue);

    /* If the change in data context is due to a removal of item from the list,
    then thew ne view model object is null. In this case, do nothing and defer
    the work for the next time an item is added: */
    if (newViewModel == nullptr)
        return;

    // make an option selected by default in the combo box
    wayptInputTypeComboBox->SelectedIndex = 0;
    wayptInputTypeComboBox->IsEnabled = true;

    static auto defBgBrush = (Media::Brush ^)this->Resources->Lookup(ref new Platform::String(L"BgColorInitialState"));

    addressTextBox->Background = defBgBrush;
    latitudeTextBox->Background = defBgBrush;
    longitudeTextBox->Background = defBgBrush;
}


/// <summary>
/// Called when the combo box selection changes the option for waypoint data input.
/// </summary>
/// <remarks>
/// This must alternate controls showing either an address or coordinates input
/// </remarks>
/// <param name="sender">The sender (combo box).</param>
/// <param name="evArgs">The event arguments.</param>
void MyWaypointControl::OnSelChangedInputOptCBox(Platform::Object ^sender, RoutedEventArgs ^evArgs)
{
    switch (OptionOfInput)
    {
    case WaypointInputOption::Address:
        addressTextBox->Visibility = Windows::UI::Xaml::Visibility::Visible;
        latitudeTextBox->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        longitudeTextBox->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        break;

    case WaypointInputOption::Coordinates:
        addressTextBox->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        latitudeTextBox->Visibility = Windows::UI::Xaml::Visibility::Visible;
        longitudeTextBox->Visibility = Windows::UI::Xaml::Visibility::Visible;
        latitudeTextBox->IsReadOnly = false;
        longitudeTextBox->IsReadOnly = false;
        break;

    case WaypointInputOption::Pointer:
        addressTextBox->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        latitudeTextBox->Visibility = Windows::UI::Xaml::Visibility::Visible;
        longitudeTextBox->Visibility = Windows::UI::Xaml::Visibility::Visible;
        latitudeTextBox->IsReadOnly = true;
        longitudeTextBox->IsReadOnly = true;
        break;

    default:
        break;
    }
}


/// <summary>
/// Called when the locate button is clicked in one of the waypoints.
/// </summary>
/// <remarks>
/// This must perform geocoding of address to a set of coordinates,
/// or reverse geocoding of a set of coordinates to an address. In
/// both cases the geographic location must be set in the map.
/// </remarks>
/// <param name="sender">The sender (locate button).</param>
/// <param name="evArgs">The event arguments.</param>
void MyWaypointControl::OnClickLocateButton(Platform::Object ^sender, RoutedEventArgs ^evArgs)
{
    auto defBgBrush = (Media::Brush ^)this->Resources->Lookup(ref new Platform::String(L"BgColorInitialState"));
    auto okayBgBrush = (Media::Brush ^)this->Resources->Lookup(ref new Platform::String(L"BgColorOkayState"));
    auto errBgBrush = (Media::Brush ^)this->Resources->Lookup(ref new Platform::String(L"BgColorErrorState"));

    addressTextBox->Background = defBgBrush;
    latitudeTextBox->Background = defBgBrush;
    longitudeTextBox->Background = defBgBrush;

    // Use reverse geocoding to validate the coordinates:
    if (OptionOfInput != WaypointInputOption::Address)
    {
        // If the input of coordinates came from user, they might be wrong:
        if (OptionOfInput == WaypointInputOption::Coordinates)
        {
            bool error(false);

            if (Latitude < -180.0 || Latitude > 180.0)
            {
                latitudeTextBox->Background = errBgBrush;
                error = true;
            }

            if (Longitude < -180.0 || Longitude > 180.0)
            {
                longitudeTextBox->Background = errBgBrush;
                error = true;
            }

            if (error)
                return;

            // while awaiting for verification, disable these:
            latitudeTextBox->IsReadOnly = true;
            longitudeTextBox->IsReadOnly = true;
        }
        // otherwise, the coordinates come from the crosshair in the map center:
        else
        {
            double latitude, longitude;

            TheMap::GetInstance().GetCenterPosition(latitude, longitude);

            Latitude = latitude;
            Longitude = longitude;
        }

        BasicGeoposition position;
        position.Altitude = 0.0;
        position.Latitude = Latitude;
        position.Longitude = Longitude;

        // while awaiting for verification, disable these:
        locateButton->IsEnabled = false;
        wayptInputTypeComboBox->IsEnabled = false;

        // Perform reverse geocoding:
        concurrency::create_task(
            MapLocationFinder::FindLocationsAtAsync(ref new Geopoint(position))
        )
        .then([this, okayBgBrush, errBgBrush](MapLocationFinderResult ^result)
        {
            if (OptionOfInput == WaypointInputOption::Coordinates)
            {
                latitudeTextBox->IsReadOnly = false;
                longitudeTextBox->IsReadOnly = false;
            }

            locateButton->IsEnabled = true;
            wayptInputTypeComboBox->IsEnabled = true;

            if (result->Status != MapLocationFinderStatus::Success
                || result->Locations->Size == 0)
            {
                latitudeTextBox->Background = errBgBrush;
                longitudeTextBox->Background = errBgBrush;
                return;
            }

            auto firstResult = result->Locations->GetAt(0);

            Address = firstResult->Address->FormattedAddress;

            // waypoint location is now verified
            m_waypoint->MarkVerified();

            TheMap::GetInstance().DisplayWaypointLocation(
                m_waypoint->Order,
                firstResult->Point->Position
            );

            addressTextBox->Background = okayBgBrush;
            latitudeTextBox->Background = okayBgBrush;
            longitudeTextBox->Background = okayBgBrush;
        });
    }
    else // Use geocoding to validate the ADDRESS:
    {
        if (m_waypoint->GetLocation().address->IsEmpty())
        {
            addressTextBox->Background = errBgBrush;
            return;
        }

        // while awaiting for verification, disable these:
        addressTextBox->IsReadOnly = true;
        locateButton->IsEnabled = false;
        wayptInputTypeComboBox->IsEnabled = false;

        // Perform geocoding:
        concurrency::create_task(
            MapLocationFinder::FindLocationsAsync(m_waypoint->GetLocation().address, m_waypoint->GeocodeQueryHint)
        )
        .then([this, okayBgBrush, errBgBrush](MapLocationFinderResult ^result)
        {
            addressTextBox->IsReadOnly = false;
            locateButton->IsEnabled = true;
            wayptInputTypeComboBox->IsEnabled = true;

            if (result->Status != MapLocationFinderStatus::Success)
            {
                addressTextBox->Background = errBgBrush;
                return;
            }

            auto firstResult = result->Locations->GetAt(0);

            Address = firstResult->Address->FormattedAddress;
            Latitude = firstResult->Point->Position.Latitude;
            Longitude = firstResult->Point->Position.Longitude;

            // waypoint location is now verified
            m_waypoint->MarkVerified();

            TheMap::GetInstance().DisplayWaypointLocation(
                m_waypoint->Order,
                firstResult->Point->Position
            );

            addressTextBox->Background = okayBgBrush;
            latitudeTextBox->Background = okayBgBrush;
            longitudeTextBox->Background = okayBgBrush;
        });
    }
}
