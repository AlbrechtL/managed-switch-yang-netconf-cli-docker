
# Managed Switch CLI/netconf/restconf Docker container based on YANG models

This project provides a Docker container for managing network switches using CLI, netconf, and restconf interfaces, leveraging YANG models for configuration and state data. It is built on top of the clixon framework and utilizes open-source YANG models from the YangModels and OpenConfig projects. The container aims to simplify deployment and integration of network management tools in a consistent, reproducible environment.

While this project is designed to be as generic as possible, development and testing have primarily focused on the [open source managed switch hardware](https://github.com/AlbrechtL/rpi-managed-switch-4-port). Compatibility with other hardware platforms may require additional adaptation or testing.

## Acknowledgements

I would like to thank the following open-source projects. Without these great works, this open-source switch would not be possible:  
* [YangModels](https://github.com/YangModels/yang)
* [OpenConfig](https://github.com/openconfig/public)
* [clixon](https://www.clicon.org/)  
* [clixon backend helper](https://github.com/MontaVista-OpenSourceTechnology/clixon-backend-helper)  
* [WebSSH](https://github.com/huashengdun/webssh)  


## Security notice
1. **Unencrypted Communication Between Web Frontend and Web Backend:**
The communication between the frontend and backend occurs over an unencrypted HTTP channel. This exposes sensitive data to potential attackers and manipulation, especially on untrusted networks. To enhance security, it is highly recommended to use a reverse proxy to upgrade the channel to HTTPS (TLS encryption) to secure communication and prevent unauthorized access.

2. **Lack of User Management in the Web Interface:**
The web interface does not include built-in user management. Any user who can access the web interface automatically control to Ethernet interface configuration. To enhance security, it is strongly recommended to place the web interface behind a reverse proxy with proper user authentication and access controls.

**Disclaimer:** This software container is a proof of concept and has not undergone comprehensive cybersecurity assessments. Users are cautioned that potential vulnerabilities may exist, posing risks to system security and data integrity. By deploying or using this container, users accept the associated risks, and the developers disclaim any responsibility for security incidents or data breaches. A thorough security evaluation, including penetration testing and compliance checks, is strongly advised before production deployment. The software is provided without warranty, and users are encouraged to provide feedback for collaborative efforts in addressing security concerns. Users acknowledge reading and understanding this disclaimer, assuming responsibility for ensuring their environment's security.
