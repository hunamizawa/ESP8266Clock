/*global $, tz_cities*/

const setting_mock = {
  "pane": "DATE_TIME",
  "override_pane": "NORMAL",
  "brightness": {
    "manual_value": -1,
    "thresholds": [1024, 360, 270, 200, 160, 120],
    "hysteresis": 10
  },
  "tzarea": "Asia",
  "tzcity": "Tokyo",
  "ntp": ["ntp.nict.jp"],
  "elev": 0,
  "use_ambient": false,
  "ambient_channelid": 100,
  "ambient_writekey": "123456789012345678",
  "use_custom_server": false,
  "custom_server_addr": "http://example.com/",
};

function changeTzCities(area) {
  $('#tzcity > option').remove();
  if (area && tz_cities[area]) {
    for (const city of tz_cities[area]) {
      $('#tzcity').append(`<option value="${city}">${city}</option>`);
    }
  }
}

function toggleVisibillityWithCheckbox(checkbox, slave) {
  if (checkbox.prop('checked'))
    slave.show();
  else
    slave.hide();
}

function assignHandlerWithCheckbox(checkbox, slave) {
  checkbox.on('change', function () {
    if (checkbox.prop('checked'))
      slave.show(500);
    else
      slave.hide(500);
  });
}

function prepareComponents() {
  $('.require-reboot').on('submit', function () {
    return confirm('設定を反映させるため、時計を再起動します。');
  });

  for (const area in tz_cities) {
    $('#tzarea').append(`<option value="${area}">${area}</option>`);
  }
  $('#tzarea').on('change', function () {
    const area = $('#tzarea > option:selected').val();
    changeTzCities(area);
  });

  $('#auto-brightness').on('change', function () {
    $('#brightness').prop('disabled', $(this).prop('checked'));
  });

  $('#panes').on('click', function (e) {
    const pane = $(e.target).val();
    if (pane)
      postSettings({ pane });
  });

  $('#override_panes').on('click', function (e) {
    const override_pane = $(e.target).val();
    if (override_pane)
      postSettings({ override_pane });
  });

  assignHandlerWithCheckbox($('#use-ambient'), $('#group-ambient-channelid'));
  assignHandlerWithCheckbox($('#use-ambient'), $('#group-ambient-writekey'));
  assignHandlerWithCheckbox($('#use-custom-server'), $('#group-custom-server-addr'));
}

function toggle_btn_group(selector, value) {
  $(`${selector} input[value=${value}]`)
    .prop('checked', true)
    .parent().addClass('active');
  $(`${selector} input[value!=${value}]`)
    .parent().removeClass('active');
}

function setSettingValues(setting) {

  $(`#tzarea > option[value=${setting.tzarea}]`).prop('selected', true);
  changeTzCities(setting.tzarea);
  $(`#tzcity > option[value=${setting.tzcity}]`).prop('selected', true);

  toggle_btn_group('#panes', setting.pane);
  toggle_btn_group('#override_panes', setting.override_pane);

  const auto_brightness = setting.brightness.manual_value == -1;
  $('#brightness').prop('disabled', auto_brightness);
  $('#auto-brightness').prop('checked', auto_brightness);

  $('#ntp1').val(setting.ntp[0]);
  if (setting.ntp.length > 1)
    $('#ntp2').val(setting.ntp[1]);
  if (setting.ntp.length > 2)
    $('#ntp3').val(setting.ntp[2]);

  $('#elev').val(setting.elev);

  $('#use-ambient').prop('checked', setting.use_ambient);
  $('#ambient-channelid').val(setting.ambient_channelid);
  $('#ambient-writekey').val(setting.ambient_writekey);

  $('#use-custom-server').prop('checked', setting.use_custom_server);
  $('#custom-server-addr').val(setting.custom_server_addr);

  toggleVisibillityWithCheckbox($('#use-ambient'), $('#group-ambient-channelid'));
  toggleVisibillityWithCheckbox($('#use-ambient'), $('#group-ambient-writekey'));
  toggleVisibillityWithCheckbox($('#use-custom-server'), $('#group-custom-server-addr'));

}

function loadAllSettings(cb) {

  $('.disable-until-load').prop('disabled', true);

  function handler(data) {
    setSettingValues(data);
    $('.disable-until-load').prop('disabled', false);
  }

  $.getJSON('/setting', handler)
    .always(() => {
      cb();
    });

  // handler(setting_mock);
  // cb();
}

function postSettings(data, cb) {

  $('.disable-until-load').prop('disabled', true);

  if (reloadTimer == null) {
    setTimeout(() => postSettings(data, cb), 100);
    return;
  }

  clearTimeout(reloadTimer);
  reloadTimer = null;

  $.ajax({
    method: 'POST',
    dataType: 'json',
    url: '/setting',
    data: data,
    success: setSettingValues
  }).always(() => {
    $('.disable-until-load').prop('disabled', false);
    reloadTimer = setTimeout(reloadDisplayMode, 1000);
    if (cb)
      cb();
  });

  // setSettingValues(setting_mock);
  // $('.disable-until-load').prop('disabled', false);
  // cb();
}

let reloadTimer = null;

function reloadDisplayMode() {

  function handler(data) {

    toggle_btn_group('#panes', data.pane);
    toggle_btn_group('#override_panes', data.override_pane);

  }

  reloadTimer = null;

  $.getJSON('/setting', handler)
    .always(() => reloadTimer = setTimeout(reloadDisplayMode, 1000));

  // handler(setting_mock);
  // reloadTimer = setTimeout(reloadDisplayMode, 1000);
}

$(function () {
  prepareComponents();
  loadAllSettings(() => {
    reloadTimer = setTimeout(reloadDisplayMode, 1000);
  });
});
