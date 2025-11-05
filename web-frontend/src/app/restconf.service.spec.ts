import { TestBed } from '@angular/core/testing';

import { RestconfService } from './restconf.service';

describe('RestconfService', () => {
  let service: RestconfService;

  beforeEach(() => {
    TestBed.configureTestingModule({});
    service = TestBed.inject(RestconfService);
  });

  it('should be created', () => {
    expect(service).toBeTruthy();
  });
});
