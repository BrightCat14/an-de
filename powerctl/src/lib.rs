use zbus::blocking::Connection;
use zbus::zvariant;

fn call_login1(method: &str, arg: Option<bool>) -> Result<(), ()> {
    let conn = Connection::system().map_err(|_| ())?;

    match arg {
        Some(v) => {
            conn.call_method(
                Some("org.freedesktop.login1"),
                             "/org/freedesktop/login1",
                             Some("org.freedesktop.login1.Manager"),
                             method,
                             &(v,),
            )
            .map_err(|_| ())?;
        }
        None => {
            conn.call_method(
                Some("org.freedesktop.login1"),
                             "/org/freedesktop/login1",
                             Some("org.freedesktop.login1.Manager"),
                             method,
                             &(),
            )
            .map_err(|_| ())?;
        }
    }

    Ok(())
}

#[no_mangle]
pub extern "C" fn powerctl_reboot() -> i32 {
    match std::panic::catch_unwind(|| call_login1("Reboot", Some(true))) {
        Ok(Ok(())) => 0,
        _ => 1,
    }
}

#[no_mangle]
pub extern "C" fn powerctl_poweroff() -> i32 {
    match std::panic::catch_unwind(|| call_login1("PowerOff", Some(true))) {
        Ok(Ok(())) => 0,
        _ => 1,
    }
}

#[no_mangle]
pub extern "C" fn powerctl_lock() -> i32 {
    match std::panic::catch_unwind(|| call_login1("LockSessions", None)) {
        Ok(Ok(())) => 0,
        _ => 1,
    }
}
