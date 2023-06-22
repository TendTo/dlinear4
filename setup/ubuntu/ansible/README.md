# Using Ansible to setup dLinear on Ubuntu

If the goal is to setup dLinear on a remote machine running Ubuntu, then Ansible can be used to automate the process in a predictable way.

## Prerequisites

- Ansible

## Setup

- Edit the `inv.yml` file to include the IP address of the remote machine
- Change the `ansible_user` to the username of the remote machine, and the directory configurations

## Run

```shell
ansible-playbook -i inv.yml --private-key <ssh_private_key> main.yml 
```
