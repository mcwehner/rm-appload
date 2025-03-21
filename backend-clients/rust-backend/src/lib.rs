use anyhow::{Error, Result};
use async_trait::async_trait;
use libc::{c_void, sockaddr_un, socket, AF_UNIX, SOCK_SEQPACKET};
use std::env::args;
use std::io;
use std::mem;
use std::mem::transmute;
use std::sync::mpsc;

#[repr(C)]
struct MessageHeader {
    msg_type: u32,
    length: u32,
}

pub struct Message {
    pub msg_type: u32,
    pub contents: String,
}

pub const MAX_PACKAGE_SIZE: usize = 10485760;
pub const MSG_SYSTEM_TERMINATE: u32 = 0xFFFFFFFF;
pub const MSG_SYSTEM_NEW_COORDINATOR: u32 = 0xFFFFFFFE;

pub struct BackendReplier {
    fd: i32,
    locked: bool,
    internal_sender: mpsc::Sender<Message>
}

impl BackendReplier {
    pub fn send_message(&self, msg_type: u32, contents: &str) -> Result<()> {
        if self.locked {
            return Err(Error::msg(
                "Cannot send back data to a terminating frontend",
            ));
        }
        send_message(self.fd, msg_type, contents)
    }

    pub fn send_internal(&self, msg_type: u32, contents: String) {
        self.internal_sender.send(Message {
            contents,
            msg_type,
        }).unwrap();
    }

    fn lock(&mut self) {
        self.locked = true;
    }
}

#[async_trait]
pub trait AppLoadBackend {
    async fn handle_message(&mut self, functionality: &BackendReplier, message: Message);
}

pub struct AppLoad<'a> {
    backend: &'a mut dyn AppLoadBackend,
    fd: i32,
    internal_channel: mpsc::Receiver<Message>,
    internal_sender: mpsc::Sender<Message>,
}

impl<'a> AppLoad<'a> {
    pub fn new(backend: &'a mut dyn AppLoadBackend) -> Result<Self> {
        let args: Vec<String> = args().collect();
        let fd = unsafe { socket(AF_UNIX, SOCK_SEQPACKET, 0) };
        if fd == -1 {
            return Err(Error::new(io::Error::last_os_error()));
        }

        let mut addr = sockaddr_un {
            sun_family: AF_UNIX as u16,
            sun_path: [0; 108],
        };
        let bytes = args[1].as_bytes();
        addr.sun_path[..bytes.len()].copy_from_slice(unsafe { transmute(bytes) });

        let connect_res = unsafe {
            libc::connect(
                fd,
                &addr as *const _ as *const libc::sockaddr,
                mem::size_of::<sockaddr_un>() as u32,
            )
        };

        if connect_res != 0 {
            return Err(Error::new(io::Error::last_os_error()));
        }

        let (s, r) = mpsc::channel();

        Ok(Self { backend, fd, internal_channel: r, internal_sender: s })
    }

    pub fn create_replier(&self) -> BackendReplier {
        BackendReplier {
            locked: false,
            fd: self.fd,
            internal_sender: self.internal_sender.clone(),
        }
    }

    pub async fn run(&mut self) -> Result<()> {
        let mut header = MessageHeader {
            length: 0,
            msg_type: 0,
        };

        let mut raw_buffer = vec![0u8; MAX_PACKAGE_SIZE];
        let mut replier = self.create_replier();

        loop {
            let mut recv_res = unsafe {
                libc::recv(
                    self.fd,
                    &mut header as *mut _ as *mut c_void,
                    mem::size_of::<MessageHeader>(),
                    0,
                )
            };

            if recv_res < 1 {
                break;
            }

            if header.length as usize > MAX_PACKAGE_SIZE {
                return Err(Error::msg("Message too exceeds protocol spec."));
            }

            recv_res = unsafe {
                libc::recv(
                    self.fd,
                    raw_buffer.as_mut_ptr() as *mut _ as *mut c_void,
                    header.length as usize,
                    0,
                )
            };

            if recv_res < 1 && header.length != 0 {
                return Err(Error::new(io::Error::last_os_error()));
            }

            let string = match header.length {
                0 => String::new(),
                len => String::from_utf8_lossy(&raw_buffer[0..len as usize]).into(),
            };
            self.backend
                .handle_message(
                    &replier,
                    Message {
                        contents: string,
                        msg_type: header.msg_type,
                    },
                )
                .await;

            for entry in self.internal_channel.try_iter() {
                self.backend.handle_message(&replier, entry).await;
            }
        }
        replier.lock();

        self.backend
            .handle_message(
                &replier,
                Message {
                    msg_type: MSG_SYSTEM_TERMINATE,
                    contents: String::default(),
                },
            )
            .await;

        Ok(())
    }
}

fn send_message(fd: i32, msg_type: u32, data: &str) -> Result<()> {
    let byte_data = data.as_bytes();
    let header = MessageHeader {
        length: byte_data.len() as u32,
        msg_type,
    };
    let mut status = unsafe {
        libc::send(
            fd,
            &header as *const _ as *const c_void,
            mem::size_of::<MessageHeader>(),
            0,
        )
    };
    if status == -1 {
        return Err(Error::new(io::Error::last_os_error()));
    }
    status = unsafe {
        if header.length > 0 {
            libc::send(
                fd,
                byte_data.as_ptr() as *const _ as *const c_void,
                header.length as usize,
                0,
            )
        } else {
            0
        }
    };
    if status == -1 {
        return Err(Error::new(io::Error::last_os_error()));
    }

    Ok(())
}
