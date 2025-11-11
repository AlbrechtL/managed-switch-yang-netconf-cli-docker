import { Component } from '@angular/core';
import { MatGridListModule } from '@angular/material/grid-list';
import { MatMenuModule } from '@angular/material/menu';
import { MatIconModule } from '@angular/material/icon';
import { MatButtonModule } from '@angular/material/button';
import { MatCardModule } from '@angular/material/card';
import { MatProgressSpinnerModule } from '@angular/material/progress-spinner';
import { MatInputModule } from '@angular/material/input';
import { MatSelectModule } from '@angular/material/select';
import { MatFormFieldModule } from '@angular/material/form-field';
import { RestconfService } from '../restconf.service';


@Component({
  selector: 'app-vlan',
    imports: [
    MatGridListModule,
    MatMenuModule,
    MatIconModule,
    MatButtonModule,
    MatCardModule,
    MatProgressSpinnerModule,
    MatInputModule,
    MatSelectModule,
    MatFormFieldModule
    ],
  templateUrl: './vlan.component.html',
  styleUrl: './vlan.component.scss'
})
export class VlanComponent{
  vlanEntries?: [string, any][];
  vlanIDs?: string[];

  interfaceModes: string[] = ['ACCESS', 'TRUNK'];
  portModeValue: string = '--';

  constructor(private service: RestconfService) { 
    service.restconfGetEthernetInterfaces().subscribe(vlanEntries => {
      this.vlanEntries = vlanEntries;
    });
    console.log('vlanEntries:', this.vlanEntries);
  }

  interfaceModeChanged(event: any, interfaceName: string) {
    console.log('Selected:', event.value);

    const xpath = `/data/openconfig-interfaces:interfaces/interface=${interfaceName}/openconfig-if-ethernet:ethernet`;
    const payload = {
      "openconfig-vlan:switched-vlan": {
        "config": {
          "interface-mode": `${event.value}`,
        }
      }
    };

    this.service.restconfPost(xpath, payload);
  }

  vlanIDsChanged(value: string, interfaceName: string, vlanMode: string) {
    console.log('Selected:', value, interfaceName, vlanMode);

    const xpath = `/data/openconfig-interfaces:interfaces/interface=${interfaceName}/openconfig-if-ethernet:ethernet`;
    const payload: any = {
      "openconfig-vlan:switched-vlan": {
      "config": {}
      }
    };

    if (vlanMode === 'ACCESS') {
      payload["openconfig-vlan:switched-vlan"].config["access-vlan"] = `${value}`;
    } else if (vlanMode === 'TRUNK') {
      payload["openconfig-vlan:switched-vlan"].config["trunk-vlans"] = `${value}`;
    }

    this.service.restconfPost(xpath, payload);
  }
}
