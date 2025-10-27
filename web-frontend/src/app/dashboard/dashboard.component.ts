import { Component } from '@angular/core';
import { MatGridListModule } from '@angular/material/grid-list';
import { MatMenuModule } from '@angular/material/menu';
import { MatIconModule } from '@angular/material/icon';
import { MatButtonModule } from '@angular/material/button';
import { MatCardModule } from '@angular/material/card';
import { MatProgressSpinnerModule } from '@angular/material/progress-spinner';
import { MatListModule } from '@angular/material/list';
import { HttpClient } from '@angular/common/http';
import { BackendCommunicationService } from '../backend-communication.service';


@Component({
  selector: 'app-dashboard',
  templateUrl: './dashboard.component.html',
  styleUrl: './dashboard.component.scss',
  standalone: true,
  imports: [
    MatGridListModule,
    MatMenuModule,
    MatIconModule,
    MatButtonModule,
    MatCardModule,
    MatProgressSpinnerModule,
    MatListModule
  ]
})
export class DashboardComponent {
  ports?: string[];

  ethernetInterfaces?: any;

  constructor(private service: BackendCommunicationService, private http: HttpClient) {
    const tmpPorts = [];
    tmpPorts.push("dummy port 1")
    tmpPorts.push("dummy port 2")

    this.ports = tmpPorts;

    // this.service.pollEthernetInterfaces().subscribe(response => {
    // })

    this.http.get<any>(
      './rest/restconf/data/openconfig-interfaces:interfaces',
      { headers: { 'Accept': 'application/yang-data+json' } }
    ).subscribe({
      next: (resp) => {
      const jsonString = JSON.stringify(resp);
      this.ethernetInterfaces = jsonString;
      const interfaces = resp?.['openconfig-interfaces:interfaces']?.interface;
      if (Array.isArray(interfaces)) {
        this.ports = interfaces
          .map((iface: any) => {
            const name = iface?.name;
            const oper = iface?.state?.['oper-status'] ?? 'unknown';
            return name ? `${name} (${oper})` : null;
          })
          .filter((n: any): n is string => !!n);
      } else {
        this.ports = [];
      }
      console.log('Ports extracted:', this.ports);
      console.log('Fetched Ethernet interfaces (JSON):', jsonString);
      },
      error: (err) => {
      console.error('Failed to fetch Ethernet interfaces:', err);
      }
    });
  }
}
