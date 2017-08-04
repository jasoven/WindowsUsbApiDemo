# WindowsUsbApiDemo

This code segment demonstrates how to register for device notification, more specifically, device notifications on insertion. This application falsely assumes any device insertion is USB-like. Utilizing this code segment may require additional work if you intend for it to work with other hardware devices.

## Getting Started

Pull the file, download the zip file, or copy the code from main.c

### Prerequisites

This code works on Windows 10. Information on MSDN incorrectly demonstrates how to register for device notification(s). The GUID used it out of date and has unnecessary components hence this application was made to demonstrate a more realistic approach.

Please ensure the appropriate libraries are included. You need to include:

* libsetupapi.a
* libwinusb.a

This application does not require administrative privileges to run.

## Built With

* [Dev C++](https://sourceforge.net/projects/orwelldevcpp/)
* [Microsoft Windows API](https://msdn.microsoft.com/en-us/library/aa383723(VS.85).aspx)

## Authors

* **Mathew A. Stefanowich** - *Initial work*

See also the list of [contributors](https://github.com/your/project/contributors) who participated in this project.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details
