# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure("2") do |config|
  config.vm.box = "bento/ubuntu-18.04"
  config.vm.define "default" do |box|
    box.vm.network "private_network", ip: "10.10.0.2"
    box.vm.network "private_network", ip: "10.10.1.2"

    box.vm.provider "virtualbox" do |v|
      v.memory = 2048
      v.cpus = 6
      v.customize ["modifyvm", :id, "--nicpromisc2", "allow-all"]
      v.customize ["modifyvm", :id, "--nicpromisc3", "allow-all"]
    end

    box.vm.provision "shell", path: "scripts/bootstrap.sh"
    box.vm.provision "shell", run: "always", path: "scripts/start_script.sh"
  end
end
