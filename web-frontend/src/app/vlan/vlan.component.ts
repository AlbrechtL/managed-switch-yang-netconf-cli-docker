import { Component } from '@angular/core';
import { MatGridListModule } from '@angular/material/grid-list';
import { MatMenuModule } from '@angular/material/menu';
import { MatIconModule } from '@angular/material/icon';
import { MatButtonModule } from '@angular/material/button';
import { MatCardModule } from '@angular/material/card';
import { MatProgressSpinnerModule } from '@angular/material/progress-spinner';
import { MatInputModule } from '@angular/material/input';
import { MatSelectModule } from '@angular/material/select';
import { MatFormFieldModule } from '@angular/material/form-field';import { HttpClient } from '@angular/common/http';

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
  //urlPrefix: string = 'http://albrecht-rpi5.local:8888'; // Just for development
  urlPrefix: string = '.'; // Just for development

  interfaceModes: string[] = ['ACCESS', 'TRUNK'];
  portModeValue: string = '--';

  constructor(private http: HttpClient) {

    // Get Ethernet interfaces and its configuration
    this.http.get<any>(
      this.urlPrefix + '/rest/restconf/data/openconfig-interfaces:interfaces',
      { headers: { 'Accept': 'application/yang-data+json' } }
    ).subscribe({
      next: (resp) => {
        const interfaces = resp?.['openconfig-interfaces:interfaces']?.interface;
        if (Array.isArray(interfaces)) {
          const vlanByInterface : Record<string, any> = interfaces.reduce((acc: Record<string, any>, value: any) => {
          const name = value?.name; // Ethernet interface name
          if (name) {
             acc[name] = value?.['openconfig-if-ethernet:ethernet']?.['openconfig-vlan:switched-vlan'] ?? null; // VLAN config for this interface
           }
            return acc;
          }, {});

          // create an iterable entries array for use in *ngFor
          this.vlanEntries = Object.entries(vlanByInterface ?? {});
          console.log('vlanEntries:', this.vlanEntries);
        }
      },
        error: (err) => {
        console.error('Failed to fetch Ethernet interfaces:', err);
      }
    });
  }
}
