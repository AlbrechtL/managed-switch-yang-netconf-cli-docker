import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable, tap, map } from 'rxjs';

interface EthernetInterfaces {
  version: string;
  kernelVersion: string;
}

@Injectable({
  providedIn: 'root'
})
export class BackendCommunicationService {
//urlPrefix: string = 'http://localhost:8888'; // Just for development
urlPrefix: string = '.'; // Relative URL

  constructor(private http: HttpClient) { }

  // getEthernetInterfaces(): Observable<EthernetInterfaces> {
  // return this.http.get<any>(this.urlPrefix + '/rest/restconf/data/openconfig-interfaces:interfaces', {headers: {'Accept': 'application/yang-data+json'}}).pipe(
  //   tap(response => console.log('Fetched data:', response)),
  //   map(response => {
  //     let tmpVersion = 'Unknown';
  //     let tmpKernelVersion = 'Unknown';
  //     if (response['combined_output'] !== '\n') {
  //       try {
  //         response['combined_output'] = JSON.parse(response['combined_output']);

  //         tmpVersion = response['combined_output']['return']['pretty-name'];
  //         tmpKernelVersion = response['combined_output']['return']['kernel-release'];
  //       } catch (error) {
  //         //console.error('Error parsing combined_output:', error);
  //       }
  //     }
  //     return {
  //       version: tmpVersion,
  //       kernelVersion: tmpKernelVersion
  //     };
  //   }));
  // }

  // pollEthernetInterfaces(intervalMs: number = 1000): Observable<EthernetInterfaces> {
  //   return new Observable(observer => {
  //     const poll = () => {
  //       this.getEthernetInterfaces().subscribe({
  //         next: (response) => {
  //           if (response.version !== 'Unknown') {
  //             observer.next(response);
  //             observer.complete();
  //           } else {
  //             setTimeout(poll, intervalMs);
  //           }
  //         },
  //         error: (error) => {
  //           observer.error(error);
  //         }
  //       });
  //     };
  //     poll();
  //   });
  // }

  commitSettings(): Observable<any> {
    return this.http.get(this.urlPrefix + '/rest/restconf/operations/ietf-netconf:commit');
  }
}
