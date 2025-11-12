import { Injectable, inject } from '@angular/core';
import { MatSnackBar } from '@angular/material/snack-bar';
import { HttpClient } from '@angular/common/http';
import { Observable, map } from 'rxjs';

@Injectable({
  providedIn: 'root'
})
export class RestconfService {
  private _snackBar = inject(MatSnackBar);

  //urlPrefix: string = '.'; // Just for development
  urlPrefix: string = 'http://localhost:8888'; // Just for development

  constructor(private http: HttpClient) { }

  restconfGetEthernetInterfaces(): Observable<[string, any][]> {
    // Return an observable of Ethernet interfaces and their VLAN configuration
    return this.http.get<any>(
      this.urlPrefix + '/rest/restconf/data/openconfig-interfaces:interfaces',
      { headers: { 'Accept': 'application/yang-data+json' } }
    ).pipe(
      map((resp) => {
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
          let vlanEntries: [string, any][] = Object.entries(vlanByInterface ?? {}); 
          return vlanEntries;
        } else {
          console.error('No interfaces found');
          return [];
        }
      })
    );
  }

  restconf(method: string, xpath: string, payload: any) {
    const url = `${this.urlPrefix}/rest/restconf/${xpath}`;

    fetch(url, {
      method: method,
      headers: { 'Content-Type': 'application/yang-data+json' },
      body: JSON.stringify(payload)
    })
    .then(async res => {
      if (!res.ok) {
        const text = await res.text().catch(() => '');

        var div = document.createElement("div");
        div.innerHTML = text; // Remove HTML tags to get error message
        console.error('Error while post', res.status, div.innerText);
        this._snackBar.open(
          'Error code ": ' + res.status + '" error text:"' + div.innerText + '"', 
          'Dismiss', 
          { panelClass: ['red-snackbar']}
        );
        return;
      }
      const data = await res.json().catch(() => null);
      console.log('Post successful', data);
      this._snackBar.open('Action successful', 'Dismiss', { duration: 2000});      
    })
    .catch(err => {
      console.error('Error while post', err);
      this._snackBar.open('Error code: ' + err, 'Dismiss', { panelClass: ['red-snackbar']});
    });
  }

  async restconfGetExists(xpath: string): Promise<boolean> {
    const url = `${this.urlPrefix}/rest/restconf/${xpath}`;
    const res = await fetch(url, {
      method: 'GET',
      headers: { 'Accept': 'application/yang-data+json' }
    });

    if (res.ok)
      return true;
    else
      return false;
  }
}
