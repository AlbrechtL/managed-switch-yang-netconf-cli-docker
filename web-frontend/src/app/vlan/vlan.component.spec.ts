import { ComponentFixture, TestBed } from '@angular/core/testing';

import { VlanComponent } from './vlan.component';

describe('VlanComponent', () => {
  let component: VlanComponent;
  let fixture: ComponentFixture<VlanComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      imports: [VlanComponent]
    })
    .compileComponents();

    fixture = TestBed.createComponent(VlanComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
