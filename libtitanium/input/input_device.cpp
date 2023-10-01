#include "input_device.hpp"
#include "input/input_actions.hpp"

#include <SDL_mouse.h>
#include <libtitanium/config/config.hpp>
#include <libtitanium/util/assert.hpp>
#include <libtitanium/util/maths.hpp>
#include <libtitanium/logger/logger.hpp>

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_keyboard.h>
#include <SDL_gamecontroller.h>

namespace input
{
    // TODO: should just be outputting i16s for analogue, so shouldn't be using float for this setting
    // maybe provide helpers for float deadzone => i16 deadzone?
    f32 s_flControllerDeadzone = 0.005; config::Var * pcvarControllerDeadzone = config::RegisterVar( "input::controllerdeadzone", config::EFVarUsageFlags::NONE, config::VARF_FLOAT, &s_flControllerDeadzone );

    void InputDevice_InitialiseKeyboard( InputDevice *const pInputDevice )
    {
        *pInputDevice = { .eDeviceType = EInputDeviceType::KEYBOARD_MOUSE, .pSDLGameController = nullptr };
    }

    bool ProcessSDLInputEvent( const SDL_Event *const pSdlEvent, util::data::Span<InputDevice> sInputDevices )
    {
        switch ( pSdlEvent->type )
        {
            case SDL_CONTROLLERDEVICEADDED:
            {
                SDL_GameController *const pSDLGameController = SDL_GameControllerOpen( pSdlEvent->cdevice.which );

                for ( uint i = 0; i < sInputDevices.m_nElements; i++ )
                {
                    if ( sInputDevices.m_pData[ i ].eDeviceType == EInputDeviceType::INVALID )
                    {
                        sInputDevices.m_pData[ i ] = { .eDeviceType = EInputDeviceType::CONTROLLER, .pSDLGameController = pSDLGameController };

                        logger::Info( "Connected new controller \"%s\" at index %i" ENDL, SDL_GameControllerName( pSDLGameController ), i );
                        return true;
                    }
                }

                logger::Info( "Tried to connect new joystick, but code has run out of controller slots! (%i available)" ENDL, sInputDevices.m_nElements );
                return true;
            }

            case SDL_CONTROLLERDEVICEREMOVED:
            {
                SDL_GameController *const pSDLGameController = SDL_GameControllerFromInstanceID( pSdlEvent->cdevice.which );

                for ( uint i = 0; i < sInputDevices.m_nElements; i++ )
                {
                    if ( sInputDevices.m_pData[ i ].eDeviceType == EInputDeviceType::CONTROLLER && sInputDevices.m_pData[ i ].pSDLGameController == pSDLGameController )
                    {
                        logger::Info( "Disconnecting controller \"%s\" from index %i" ENDL, SDL_GameControllerName( pSDLGameController ), i );
                        SDL_GameControllerClose( pSDLGameController );
                        sInputDevices.m_pData[ i ] = { .eDeviceType = EInputDeviceType::INVALID, .pSDLGameController = nullptr };
                        return true;
                    }
                }

                // shouldn't be possible to hit
                return true;
            }

            case SDL_MOUSEMOTION:
            {

            }

            case SDL_MOUSEWHEEL:
            {

            }

            default:
            {
                return false;
            }
        }
    }

    constexpr i16 MAX_JOYSTICK = maxof( i16 );
    constexpr i16 MIN_JOYSTICK = -MAX_JOYSTICK;

    void ProcessAnalogueActions( util::data::Span<InputDevice> sMulticastInputDevices, const util::data::Span<AnalogueBinding> sInputBindings, util::data::Span<i16> o_snAnalogueActionState )
    {
        assert::Debug( sInputBindings.m_nElements == o_snAnalogueActionState.m_nElements );
        memset( o_snAnalogueActionState.m_pData, 0, o_snAnalogueActionState.m_nElements * sizeof( i16 ) );

        for ( uint i = 0; i < sMulticastInputDevices.m_nElements; i++ )
        {
            InputDevice *const pInputDevice = &sMulticastInputDevices.m_pData[ i ];
            if ( pInputDevice->eDeviceType == EInputDeviceType::INVALID )
            {
                continue;
            }

            for ( uint j = 0; j < sInputBindings.m_nElements; j++ )
            {
                i16 nValue = 0;

                if ( pInputDevice->eDeviceType == EInputDeviceType::KEYBOARD_MOUSE )
                {
                    // TODO: mouse button and axis support
                    const u8* const pbSDLKeyStates = SDL_GetKeyboardState( nullptr );
                    const u32 nMouseStateBits = SDL_GetMouseState( nullptr, nullptr );

                    EKeyboardMouseButton ePosButton = sInputBindings.m_pData[ j ].eKBButtonPos;
                    if ( EKeyboardMouseButton_IsMouseInput( ePosButton ) )
                    {
                    }
                    else
                    {
                        if ( pbSDLKeyStates[ EKeyboardMouseButton_ToSDLKeyboardScancode( ePosButton ) ] )
                        {
                            nValue += MAX_JOYSTICK;
                        }
                    }

                    EKeyboardMouseButton eNegButton = sInputBindings.m_pData[ j ].eKBButtonNeg;
                    if ( EKeyboardMouseButton_IsMouseInput( eNegButton ) )
                    {
                    }
                    else
                    {
                        if ( pbSDLKeyStates[ EKeyboardMouseButton_ToSDLKeyboardScancode( eNegButton ) ] )
                        {
                            nValue += MIN_JOYSTICK;
                        }
                    }
                }
                else if ( pInputDevice->eDeviceType == EInputDeviceType::CONTROLLER )
                {
                    if ( SDL_GameControllerGetButton( pInputDevice->pSDLGameController, EControllerButtonToSDLButton( sInputBindings.m_pData[ j ].eControllerButtonPos ) ) )
                    {
                        nValue += MAX_JOYSTICK;
                    }
                    else if ( SDL_GameControllerGetButton( pInputDevice->pSDLGameController, EControllerButtonToSDLButton(sInputBindings.m_pData[ j ].eControllerButtonNeg ) ) )
                    {
                        nValue += MIN_JOYSTICK;
                    }
                    else
                    {
                        SDL_GameControllerAxis sdlGameControllerAxis = EControllerAxisToSDLAxis( sInputBindings.m_pData[ j ].eControllerAxis );
                        nValue = SDL_GameControllerGetAxis( pInputDevice->pSDLGameController, sdlGameControllerAxis );

                        if ( abs( nValue ) < MAX_JOYSTICK * s_flControllerDeadzone )
                        {
                            nValue = 0;
                        }
                        // SDL gives joystick y values as negative-up, we use positive-up y values for axis, so reverse them
                        else if ( sdlGameControllerAxis == SDL_CONTROLLER_AXIS_LEFTY || sdlGameControllerAxis == SDL_CONTROLLER_AXIS_RIGHTY )
                        {
                            nValue = -nValue;
                        }
                    }

                }

                if ( abs( nValue ) > abs( o_snAnalogueActionState.m_pData[ j ] ) )
                {
                    o_snAnalogueActionState.m_pData[ j ] = nValue;
                }
            }
        }
    }

    void ProcessDigitalActions( util::data::Span<InputDevice> sMulticastInputDevices, const util::data::Span<DigitalBinding> sInputBindings, util::data::Span<u8> o_snDigitalActionState )
    {
        assert::Debug( util::maths::WithinRange( sInputBindings.m_nElements, SizeNeededForDigitalActions( o_snDigitalActionState.m_nElements ), SizeNeededForDigitalActions( o_snDigitalActionState.m_nElements ) + 1 ) );

        for ( uint i = 0; i < sInputBindings.m_nElements; i++ )
        {
            bool bPreviouslyHeld = DigitalActionHeld( o_snDigitalActionState, i );

            bool bCurrentlyHeld = false;
            for ( uint j = 0; j < sMulticastInputDevices.m_nElements; j++ )
            {
                InputDevice *const pInputDevice = &sMulticastInputDevices.m_pData[ j ];
                if ( pInputDevice->eDeviceType == EInputDeviceType::INVALID )
                {
                    continue;
                }

                if ( pInputDevice->eDeviceType == EInputDeviceType::KEYBOARD_MOUSE )
                {
                    // TODO: mouse button and axis support

                    const u8* const pbSDLKeyStates = SDL_GetKeyboardState( nullptr );
                    bCurrentlyHeld = pbSDLKeyStates[ EKeyboardMouseButton_ToSDLKeyboardScancode( sInputBindings.m_pData[ i ].eKBButton ) ];
                }
                else if ( pInputDevice->eDeviceType == EInputDeviceType::CONTROLLER )
                {
                    if ( SDL_GameControllerGetButton( pInputDevice->pSDLGameController, EControllerButtonToSDLButton( sInputBindings.m_pData[ i ].eControllerButton ) ) )
                    {
                        bCurrentlyHeld = true;
                    }
                    else
                    {
                        i16 nRawValue = SDL_GameControllerGetAxis( pInputDevice->pSDLGameController, EControllerAxisToSDLAxis( sInputBindings.m_pData[ i ].eControllerAxis ) );
                        bCurrentlyHeld = abs( nRawValue ) >= MAX_JOYSTICK * s_flControllerDeadzone;
                    }
                }

                if ( bCurrentlyHeld )
                    break;
            }

            if ( bCurrentlyHeld )
            {
                if ( bPreviouslyHeld )
                {
                    SetDigitalActionHeld( o_snDigitalActionState, i );
                }
                else
                {
                    SetDigitalActionPressed( o_snDigitalActionState, i );
                }
            }
            else
            {
                if ( bPreviouslyHeld )
                {
                    SetDigitalActionDepressed( o_snDigitalActionState, i );
                }
                else
                {
                    SetDigitalActionClear( o_snDigitalActionState, i );
                }
            }
        }
    }
}
